// AI Client Daemon - 与AI服务器通信
// 西游记MUD AI NPC系统
// 支持记忆持久化和玩家关系

#include <ansi.h>
#include <net/socket.h>

// 配置
#define AI_HOST "127.0.0.1"
#define AI_PORT 9999
#define TIMEOUT 30

// 数据目录
#define RELATION_DIR "/data/ai_relation/"
#define HISTORY_DIR "/data/ai_history/"

// 关系等级定义
#define RELATION_ENEMY    -80   // 仇敌
#define RELATION_HOSTILE  -50   // 敌对
#define RELATION_COLD     -20   // 疏远
#define RELATION_STRANGER 0     // 陌生人
#define RELATION_ACQUAINT 20    // 相识
#define RELATION_FRIEND   50    // 友好
#define RELATION_CLOSE    80    // 挚友

// 全局变量
nosave mapping npc_contexts;  // NPC上下文记忆(运行时缓存)
nosave mapping sockets;       // socket信息
nosave mapping relation_cache; // 关系缓存

void create()
{
    seteuid(getuid());
    npc_contexts = ([]);
    sockets = ([]);
    relation_cache = ([]);

    // 确保数据目录存在（file_size返回-2表示目录存在）
    if (file_size(RELATION_DIR) == -1)
        mkdir("data/ai_relation");
    if (file_size(HISTORY_DIR) == -1)
        mkdir("data/ai_history");
}

// ==================== 关系系统 ====================

// 获取关系等级名称
string get_relation_title(int relation)
{
    if (relation >= RELATION_CLOSE) return "挚友";
    if (relation >= RELATION_FRIEND) return "友好";
    if (relation >= RELATION_ACQUAINT) return "相识";
    if (relation >= RELATION_COLD) return "陌生人";
    if (relation >= RELATION_HOSTILE) return "疏远";
    if (relation >= RELATION_ENEMY) return "敌对";
    return "仇敌";
}

// 获取关系文件路径
string get_relation_file(string npc_id, string player_id)
{
    string npc_name = replace_string(npc_id, "/", "_");
    return sprintf("%s%s/%s.o", RELATION_DIR, npc_name, player_id);
}

// 加载玩家与NPC的关系
mapping load_relation(string npc_id, string player_id)
{
    string file = get_relation_file(npc_id, player_id);
    mapping rel;

    // 先检查缓存
    string cache_key = npc_id + "#" + player_id;
    if (relation_cache[cache_key]) {
        return relation_cache[cache_key];
    }

    // 从文件加载
    if (file_size(file) >= 0) {
        rel = restore_variable(read_file(file));
    } else {
        rel = ([
            "value": 0,           // 关系值 -100到100
            "meet_count": 0,      // 见面次数
            "talk_count": 0,      // 对话次数
            "quest_count": 0,     // 完成任务次数
            "gift_count": 0,      // 送礼次数
            "fight_count": 0,     // 战斗次数
            "first_meet": 0,      // 首次见面时间
            "last_meet": 0,       // 最后见面时间
            "important_events": ({}), // 重要事件记录
        ]);
    }

    // 缓存
    relation_cache[cache_key] = rel;
    return rel;
}

// 保存关系
void save_relation(string npc_id, string player_id, mapping rel)
{
    string file = get_relation_file(npc_id, player_id);
    string dir = sprintf("%s%s", RELATION_DIR, replace_string(npc_id, "/", "_"));
    string cache_key = npc_id + "#" + player_id;

    // 确保目录存在
    if (file_size(dir) == -1)
        mkdir(dir);

    // 写入文件
    assure_file(file);
    write_file(file, save_variable(rel), 1);

    // 更新缓存
    relation_cache[cache_key] = rel;
}

// 更新关系值
int update_relation(string npc_id, string player_id, int delta, string reason)
{
    mapping rel = load_relation(npc_id, player_id);
    int old_value = rel["value"];
    int new_value = old_value + delta;

    // 限制范围
    if (new_value > 100) new_value = 100;
    if (new_value < -100) new_value = -100;

    rel["value"] = new_value;

    // 记录重要变化
    if (abs(delta) >= 10 && sizeof(rel["important_events"]) < 50) {
        rel["important_events"] += ({
            sprintf("%s|%d|%s", ctime(time())[0..9], delta, reason)
        });
    }

    save_relation(npc_id, player_id, rel);

    // 返回是否跨越关系等级
    string old_title = get_relation_title(old_value);
    string new_title = get_relation_title(new_value);
    return (old_title != new_title) ? 1 : 0;
}

// 记录见面
void record_meeting(string npc_id, string player_id, string player_name)
{
    mapping rel = load_relation(npc_id, player_id);

    if (rel["first_meet"] == 0) {
        rel["first_meet"] = time();
        // 首次见面+5关系
        rel["value"] += 5;
    }

    rel["meet_count"]++;
    rel["last_meet"] = time();

    save_relation(npc_id, player_id, rel);
}

// 记录对话
void record_talk(string npc_id, string player_id)
{
    mapping rel = load_relation(npc_id, player_id);
    rel["talk_count"]++;

    // 每10次对话+1关系（最多+10）
    if (rel["talk_count"] <= 100 && rel["talk_count"] % 10 == 0) {
        rel["value"]++;
    }

    save_relation(npc_id, player_id, rel);
}

// 记录完成任务
void record_quest(string npc_id, string player_id, string quest_name)
{
    mapping rel = load_relation(npc_id, player_id);
    rel["quest_count"]++;
    rel["value"] += 10;  // 完成任务+10关系

    // 记录事件
    if (sizeof(rel["important_events"]) < 50) {
        rel["important_events"] += ({
            sprintf("%s|+10|完成任务:%s", ctime(time())[0..9], quest_name)
        });
    }

    save_relation(npc_id, player_id, rel);
}

// 记录送礼
void record_gift(string npc_id, string player_id, string gift_name, int value)
{
    mapping rel = load_relation(npc_id, player_id);
    rel["gift_count"]++;
    rel["value"] += value;  // 根据礼物价值增加关系

    if (sizeof(rel["important_events"]) < 50) {
        rel["important_events"] += ({
            sprintf("%s|+%d|赠送:%s", ctime(time())[0..9], value, gift_name)
        });
    }

    save_relation(npc_id, player_id, rel);
}

// 记录被玩家杀死（关系变成仇敌）
void record_killed_by_player(string npc_id, string player_id, string player_name)
{
    mapping rel = load_relation(npc_id, player_id);
    int old_value = rel["value"];

    // 设置为仇敌
    rel["value"] = -100;
    rel["fight_count"]++;

    // 记录重要事件
    if (sizeof(rel["important_events"]) < 50) {
        rel["important_events"] += ({
            sprintf("%s|-100|被%s杀死，成为仇敌", ctime(time())[0..9], player_name || player_id)
        });
    }

    save_relation(npc_id, player_id, rel);

    // 记录日志
    log_file("ai_relation", sprintf("[%s] %s killed by %s, relation: %d -> -100\n",
        ctime(time())[0..9], npc_id, player_id, old_value));
}

// 构建关系描述（用于AI提示）
string build_relation_context(string npc_id, string player_id, string player_name)
{
    mapping rel = load_relation(npc_id, player_id);
    int value = rel["value"];
    string title = get_relation_title(value);
    string context = "";

    context += sprintf("【玩家关系】\n");
    context += sprintf("- 玩家：%s\n", player_name);
    context += sprintf("- 关系：%s（%d）\n", title, value);

    if (rel["meet_count"] > 0) {
        context += sprintf("- 见面次数：%d次\n", rel["meet_count"]);
    }

    if (rel["talk_count"] > 0) {
        context += sprintf("- 对话次数：%d次\n", rel["talk_count"]);
    }

    if (rel["quest_count"] > 0) {
        context += sprintf("- 完成任务：%d次\n", rel["quest_count"]);
    }

    // 根据关系值调整语气建议
    context += "\n【回复风格建议】\n";
    if (value >= RELATION_CLOSE) {
        context += "- 这是你的挚友，语气要亲切热情\n";
        context += "- 可以使用昵称或亲密的称呼\n";
    } else if (value >= RELATION_FRIEND) {
        context += "- 这是你的朋友，语气友好\n";
        context += "- 可以表现出信任和关心\n";
    } else if (value >= RELATION_ACQUAINT) {
        context += "- 这是熟人，语气自然\n";
        context += "- 可以简单打招呼\n";
    } else if (value >= RELATION_COLD) {
        context += "- 这是陌生人，语气礼貌但保持距离\n";
    } else if (value >= RELATION_HOSTILE) {
        context += "- 你不喜欢这个人，语气冷淡\n";
    } else {
        context += "- 这是你的敌人，语气警惕或敌对\n";
    }

    return context;
}

// ==================== 对话历史系统 ====================

// 获取历史文件路径
string get_history_file(string npc_id, string player_id)
{
    string npc_name = replace_string(npc_id, "/", "_");
    return sprintf("%s%s/%s.o", HISTORY_DIR, npc_name, player_id);
}

// 加载对话历史
mixed *load_history(string npc_id, string player_id)
{
    string file = get_history_file(npc_id, player_id);

    if (file_size(file) >= 0) {
        return restore_variable(read_file(file));
    }

    return ({});
}

// 保存对话历史
void save_history(string npc_id, string player_id, mixed *history)
{
    string file = get_history_file(npc_id, player_id);
    string dir = sprintf("%s%s", HISTORY_DIR, replace_string(npc_id, "/", "_"));

    if (file_size(dir) == -1)
        mkdir(dir);

    assure_file(file);
    write_file(file, save_variable(history), 1);
}

// 添加对话到历史
void add_to_history(object npc, object player, string role, string content)
{
    string npc_id = base_name(npc);
    string player_id = player->query("id");
    mixed *history = load_history(npc_id, player_id);

    // 添加新对话
    history += ({ ([
        "time": time(),
        "role": role,
        "content": content
    ]) });

    // 保留最近30轮对话（60条消息）
    if (sizeof(history) > 60) {
        history = history[<60..];
    }

    save_history(npc_id, player_id, history);

    // 同时更新运行时缓存
    string cache_key = npc_id + "#" + player_id;
    if (!npc_contexts[cache_key]) {
        npc_contexts[cache_key] = (["history": ({}), "personality": npc->query("ai_personality")]);
    }
    npc_contexts[cache_key]["history"] = history;
}

// 构建对话历史（用于AI请求）
mixed *build_messages_for_ai(string npc_id, string player_id)
{
    mixed *history = load_history(npc_id, player_id);
    mixed *messages = ({});
    int i;

    // 只取最近的对话
    int start = 0;
    if (sizeof(history) > 20) {
        start = sizeof(history) - 20;
    }

    for (i = start; i < sizeof(history); i++) {
        messages += ({ (["role": history[i]["role"], "content": history[i]["content"]]) });
    }

    return messages;
}

// 获取NPC上下文（兼容旧接口）
mapping get_npc_context(object npc)
{
    string npc_id = base_name(npc);

    if (!npc_contexts[npc_id]) {
        npc_contexts[npc_id] = ([
            "history": ({}),
            "personality": npc->query("ai_personality") || "你是一个普通的NPC。",
            "name": npc->query("name"),
        ]);
    }

    return npc_contexts[npc_id];
}

// ==================== AI请求系统 ====================

// 构建系统提示（增强版）
string build_system_prompt(object npc, object player)
{
    string npc_id = base_name(npc);
    string player_id = player->query("id");
    string player_name = player->query("name");
    string personality = npc->query("ai_personality") || "你是一个普通的NPC。";
    string name = npc->query("name");

    // 获取关系上下文
    string relation_context = build_relation_context(npc_id, player_id, player_name);

    return sprintf(
        "你是西游记MUD游戏中的NPC「%s」。\n\n"
        "【人物设定】\n%s\n\n"
        "%s\n"
        "【回复规则】\n"
        "1. 用中文回复，语气符合人物设定和关系远近\n"
        "2. 回复简短自然，像游戏对话(1-3句话)\n"
        "3. 可以在回复开头用【动作】格式添加动作，如【点头】\n"
        "4. 保持角色一致性，记住之前的对话\n"
        "5. 根据与玩家的关系调整语气和态度",
        name, personality, relation_context
    );
}

// 简单JSON编码
string json_encode(mixed data)
{
    if (intp(data)) return "" + data;
    if (stringp(data)) {
        data = replace_string(data, "\\", "\\\\");
        data = replace_string(data, "\"", "\\\"");
        data = replace_string(data, "\n", "\\n");
        data = replace_string(data, "\r", "\\r");
        data = replace_string(data, "\t", "\\t");
        return "\"" + data + "\"";
    }

    if (arrayp(data)) {
        string result = "[";
        for (int i = 0; i < sizeof(data); i++) {
            if (i > 0) result += ",";
            result += json_encode(data[i]);
        }
        return result + "]";
    }

    if (mapp(data)) {
        string result = "{";
        mixed *ks = keys(data);
        for (int i = 0; i < sizeof(ks); i++) {
            if (i > 0) result += ",";
            result += "\"" + ks[i] + "\":" + json_encode(data[ks[i]]);
        }
        return result + "}";
    }

    return "null";
}

// 简单JSON解析（提取字段）
string extract_json_string(string json, string field)
{
    string search = "\"" + field + "\"";
    int pos = strsrch(json, search);
    if (pos < 0) return 0;

    int colon_pos = strsrch(json[pos..], ":");
    if (colon_pos < 0) return 0;
    colon_pos += pos;

    int start = colon_pos + 1;
    while (start < strlen(json) && (json[start] == ' ' || json[start] == '\t')) {
        start++;
    }
    if (start >= strlen(json) || json[start] != '"') return 0;
    start++;

    int end = start;
    while (end < strlen(json)) {
        if (json[end] == '"' && json[end-1] != '\\') {
            break;
        }
        end++;
    }

    if (end >= strlen(json)) return 0;

    string result = json[start..end-1];
    result = replace_string(result, "\\n", "\n");
    result = replace_string(result, "\\r", "\r");
    result = replace_string(result, "\\t", "\t");
    result = replace_string(result, "\\\"", "\"");
    result = replace_string(result, "\\\\", "\\");

    return result;
}

// Socket写回调
void write_callback(int fd)
{
    if (!sockets[fd]) return;

    string data = sockets[fd]["outgoing"];
    log_file("ai_client", sprintf("write_callback fd=%d, data_len=%d\n", fd, strlen(data || "")));

    if (data && strlen(data) > 0) {
        int result = socket_write(fd, data);
        log_file("ai_client", sprintf("socket_write result: %d\n", result));
        if (result == EESUCCESS || result == EECALLBACK) {
            sockets[fd]["outgoing"] = "";
        }
    }
}

// Socket读回调
void read_callback(int fd, string data)
{
    log_file("ai_client", sprintf("*** read_callback fd=%d, data_len=%d ***\n", fd, strlen(data || "")));

    if (!sockets[fd]) {
        log_file("ai_client", "read_callback: sockets[fd] is null!\n");
        return;
    }

    sockets[fd]["response"] += data;

    string response = sockets[fd]["response"];
    log_file("ai_client", sprintf("Total response: %d bytes\n", strlen(response)));

    if (strsrch(response, "}") >= 0 && response[<1] == '}') {
        string reply = extract_json_string(response, "reply");
        log_file("ai_client", sprintf("Parsed reply: %s\n", reply ? reply : "null"));

        if (reply && strlen(reply) > 0) {
            object npc = sockets[fd]["npc"];
            object player = sockets[fd]["player"];
            string input = sockets[fd]["input"];

            if (npc && player) {
                string npc_id = base_name(npc);
                string player_id = player->query("id");

                string action = "";
                string reply_text = reply;

                int action_start = strsrch(reply, "【");
                int action_end = strsrch(reply, "】");
                if (action_start >= 0 && action_end > action_start) {
                    action = reply[action_start + 1 .. action_end - 1];
                    reply_text = reply[action_end + 1..];

                    if (strlen(action) > 0 && npc) {
                        npc->command(action);
                    }
                }

                // 保存对话历史
                add_to_history(npc, player, "user", input);
                add_to_history(npc, player, "assistant", reply);

                // 记录对话
                record_talk(npc_id, player_id);

                if (strlen(reply_text) > 0) {
                    message_vision(CYN "$N说道：" + reply_text + "\n" NOR, npc);
                }
            }
        }

        map_delete(sockets, fd);
        socket_close(fd);
    }
}

// Socket关闭回调
void close_callback(int fd)
{
    log_file("ai_client", sprintf("close_callback fd=%d\n", fd));
    if (sockets[fd]) {
        string resp = sockets[fd]["response"];
        if (!resp || strlen(resp) == 0) {
            object npc = sockets[fd]["npc"];
            if (npc) {
                message_vision(CYN "$N似乎在想什么...\n" NOR, npc);
            }
        }
        map_delete(sockets, fd);
    }
}

// 发送AI请求
int send_ai_request(object npc, object player, string input)
{
    int fd;
    string request_json;
    string system_prompt;
    mixed *messages;
    mapping msg;
    mapping req;

    fd = socket_create(STREAM, "read_callback", "close_callback");
    if (fd < 0) {
        log_file("ai_client", sprintf("Failed to create socket: %d\n", fd));
        return 0;
    }

    string npc_id = base_name(npc);
    string player_id = player->query("id");

    log_file("ai_client", sprintf("Socket created: fd=%d, npc=%s, player=%s\n", fd, npc_id, player_id));

    sockets[fd] = ([
        "npc": npc,
        "player": player,
        "input": input,
        "response": "",
        "outgoing": "",
        "time": time()
    ]);

    int result = socket_connect(fd, AI_HOST + " " + AI_PORT, "read_callback", "write_callback");
    log_file("ai_client", sprintf("Connect result: %d\n", result));

    if (result != EESUCCESS && result != EECALLBACK) {
        log_file("ai_client", sprintf("Failed to connect: %d\n", result));
        map_delete(sockets, fd);
        socket_close(fd);
        return 0;
    }

    // 构建请求
    system_prompt = build_system_prompt(npc, player);
    messages = build_messages_for_ai(npc_id, player_id);
    messages += ({ (["role": "user", "content": input]) });

    req = ([
        "system": system_prompt,
        "messages": messages,
        "max_tokens": 500,
        "timeout": TIMEOUT
    ]);
    request_json = json_encode(req);

    sockets[fd]["outgoing"] = request_json;
    log_file("ai_client", sprintf("Request ready, len=%d\n", strlen(request_json)));

    if (result == EESUCCESS) {
        write_callback(fd);
    }

    call_out("timeout_callback", TIMEOUT, fd);

    return 1;
}

void timeout_callback(int fd)
{
    log_file("ai_client", sprintf("timeout_callback fd=%d\n", fd));
    if (sockets[fd]) {
        object npc = sockets[fd]["npc"];
        string resp = sockets[fd]["response"];

        if (npc && (!resp || strlen(resp) == 0)) {
            message_vision(CYN "$N似乎在想什么...\n" NOR, npc);
        }

        map_delete(sockets, fd);
        socket_close(fd);
    }
}

// 处理ask命令
int process_ask(object player, object npc, string topic)
{
    string npc_id = base_name(npc);
    string player_id = player->query("id");
    string player_name = player->query("name");
    string input = sprintf("%s问：%s", player_name, topic);

    // 记录见面
    record_meeting(npc_id, player_id, player_name);

    message_vision(CYN "$N若有所思...\n" NOR, npc, player);

    return send_ai_request(npc, player, input);
}

// 清除NPC记忆
int clear_npc_memory(object npc, string player_id)
{
    string npc_id = base_name(npc);
    string cache_key = npc_id + "#" + player_id;

    // 清除缓存
    map_delete(npc_contexts, cache_key);
    map_delete(relation_cache, cache_key);

    // 清除文件
    string hist_file = get_history_file(npc_id, player_id);
    string rel_file = get_relation_file(npc_id, player_id);

    if (file_size(hist_file) >= 0) rm(hist_file);
    if (file_size(rel_file) >= 0) rm(rel_file);

    return 1;
}

// 查询玩家与NPC的关系
int query_relation(string npc_id, string player_id)
{
    mapping rel = load_relation(npc_id, player_id);
    return rel["value"];
}

// 查询关系详情
mapping query_relation_detail(string npc_id, string player_id)
{
    return load_relation(npc_id, player_id);
}

// ==================== 自主行动系统 ====================

string build_autonomous_prompt(object npc)
{
    mapping ctx = get_npc_context(npc);
    string personality = ctx["personality"];
    string name = npc->query("name");
    object env = environment(npc);
    string room_desc = "";
    string people_desc = "";
    object *inv;
    object *people;
    object *players;
    int i;

    if (env) {
        room_desc = env->query("short") || "";
        inv = all_inventory(env);
        people = ({});
        players = ({});

        for (i = 0; i < sizeof(inv); i++) {
            if (living(inv[i]) && inv[i] != npc && userp(inv[i])) {
                players += ({ inv[i] });
            }
        }

        for (i = 0; i < sizeof(inv); i++) {
            if (living(inv[i]) && inv[i] != npc && !userp(inv[i])) {
                people += ({ inv[i] });
            }
        }

        people = players + people;

        if (sizeof(people) > 0) {
            people_desc = "周围有：";
            for (i = 0; i < sizeof(people) && i < 5; i++) {
                people_desc += people[i]->query("name") + "(" + people[i]->query("id") + ")";
                if (userp(people[i])) people_desc += "[玩家]";
                if (i < sizeof(people) - 1 && i < 4) people_desc += "、";
            }
        }
    }

    string npc_id = npc->query("id");
    string action_prompt = "";

    if (npc_id == "renwu" || npc_id == "shizhe" || npc_id == "tasker" ||
        strsrch(personality, "任务") >= 0 || strsrch(personality, "发布任务") >= 0) {
        action_prompt = "重要：你是任务使者，主要目的是发布任务！\n"
            "- 如果周围有玩家，使用【说话】提醒他们接任务\n"
            "- 偶尔使用【发布任务】广播任务提示\n\n"
            "行动类型：\n"
            "【说话】内容 - 主动说一句话\n"
            "【发布任务】 - 提醒玩家接任务\n"
            "【移动】方向 - 移动\n"
            "【休息】 - 原地休息\n";
    } else {
        action_prompt = "重要：你是武痴，主要目的是找人切磋武艺！\n"
            "- 如果周围有人，优先使用【挑战】发起战斗\n"
            "- 【挑战】格式：目标玩家的英文ID\n\n"
            "行动类型：\n"
            "【挑战】目标ID - 向目标发起挑战\n"
            "【说话】内容 - 主动说一句话\n"
            "【移动】方向 - 移动\n"
            "【休息】 - 原地休息\n";
    }

    return sprintf(
        "你是西游记MUD游戏中的NPC「%s」。\n\n"
        "人物设定：\n%s\n\n"
        "当前环境：\n房间：%s\n%s\n\n"
        "请决定你接下来要做什么，只能做一个行动。\n"
        "返回格式：【行动类型】行动数据\n\n%s",
        name, personality, room_desc, people_desc, action_prompt
    );
}

void process_autonomous_response(int fd, string response)
{
    string reply = extract_json_string(response, "reply");
    string action_type, action_data;
    object npc;

    if (!reply || strlen(reply) == 0) {
        return;
    }

    npc = sockets[fd]["npc"];
    if (!npc) return;

    int action_start = strsrch(reply, "【");
    int action_end = strsrch(reply, "】");

    if (action_start >= 0 && action_end > action_start) {
        action_type = reply[action_start + 1 .. action_end - 1];
        action_data = reply[action_end + 1..];

        action_data = replace_string(action_data, "  ", " ");
        action_data = replace_string(action_data, " ", " ");

        if (npc->execute_ai_action(action_type, action_data)) {
            log_file("ai_client", sprintf("action: %s - %s\n", action_type, action_data));
        }
    }
}

void autonomous_read_callback(int fd, string data)
{
    if (!sockets[fd]) return;

    sockets[fd]["response"] += data;

    string response = sockets[fd]["response"];
    if (strsrch(response, "}") >= 0 && response[<1] == '}') {
        process_autonomous_response(fd, response);
        map_delete(sockets, fd);
        socket_close(fd);
    }
}

int process_autonomous_action(object npc)
{
    int fd;
    string request_json;
    string system_prompt;
    mapping msg;
    mapping req;

    if (!npc || !living(npc)) return 0;

    fd = socket_create(STREAM, "autonomous_read_callback", "close_callback");
    if (fd < 0) return 0;

    sockets[fd] = ([
        "npc": npc,
        "response": "",
        "outgoing": "",
        "time": time()
    ]);

    int result = socket_connect(fd, AI_HOST + " " + AI_PORT, "autonomous_read_callback", "write_callback");
    if (result != EESUCCESS && result != EECALLBACK) {
        map_delete(sockets, fd);
        socket_close(fd);
        return 0;
    }

    system_prompt = build_autonomous_prompt(npc);

    msg = (["role": "user", "content": "请决定你的下一个行动："]);
    req = ([
        "system": system_prompt,
        "messages": ({msg}),
        "max_tokens": 100,
        "timeout": TIMEOUT
    ]);
    request_json = json_encode(req);

    sockets[fd]["outgoing"] = request_json;

    if (result == EESUCCESS) {
        write_callback(fd);
    }

    call_out("timeout_callback", TIMEOUT, fd);

    return 1;
}

// ==================== 李白自动吟诗系统 ====================

// 构建吟诗环境描述
string build_poem_environment(object npc)
{
    object env = environment(npc);
    string desc = "";
    string room_name, room_long;
    string *exits;
    object *inv;
    object *people;
    int i;
    mixed *time_info;
    int hour;
    string time_desc;

    if (!env) return "未知环境";

    // 房间信息
    room_name = env->query("short") || "某处";
    room_long = env->query("long") || "";
    exits = keys(env->query("exits") || ({}));

    desc = sprintf("地点：%s\n", room_name);

    // 房间描述（取前200字）
    if (strlen(room_long) > 0) {
        if (strlen(room_long) > 200) {
            room_long = room_long[0..199] + "...";
        }
        desc += sprintf("环境：%s\n", room_long);
    }

    // 出口信息
    if (sizeof(exits) > 0) {
        desc += sprintf("出口：%s\n", implode(exits, "、"));
    }

    // 时间信息
    time_info = NATURE_D->query_time();
    if (sizeof(time_info) >= 2) {
        hour = time_info[1];
        if (hour >= 5 && hour < 8) time_desc = "清晨";
        else if (hour >= 8 && hour < 12) time_desc = "上午";
        else if (hour >= 12 && hour < 14) time_desc = "正午";
        else if (hour >= 14 && hour < 17) time_desc = "下午";
        else if (hour >= 17 && hour < 19) time_desc = "傍晚";
        else if (hour >= 19 && hour < 22) time_desc = "夜晚";
        else time_desc = "深夜";

        desc += sprintf("时间：%s\n", time_desc);

        // 夜晚特别提示
        if (hour >= 19 || hour < 5) {
            desc += "注意：此时明月当空，适合月下独酌。\n";
        }
    }

    // 周围人物
    inv = all_inventory(env);
    people = ({});
    for (i = 0; i < sizeof(inv); i++) {
        if (living(inv[i]) && inv[i] != npc && userp(inv[i])) {
            people += ({inv[i]});
        }
    }

    if (sizeof(people) > 0) {
        desc += "周围有人：";
        for (i = 0; i < sizeof(people) && i < 3; i++) {
            desc += people[i]->query("name");
            if (i < sizeof(people) - 1 && i < 2) desc += "、";
        }
        desc += "\n";
    } else {
        desc += "周围无人，只有自己。\n";
    }

    // NPC自身状态
    int water = npc->query("water") || 0;
    int max_water = npc->query("max_water") || 400;
    if (water < max_water / 3) {
        desc += "状态：有点口渴，想喝酒。\n";
    }

    return desc;
}

// 构建吟诗prompt
string build_poem_prompt(object npc)
{
    string env_desc = build_poem_environment(npc);
    string name = npc->query("name");

    return sprintf(
        "你是唐朝诗人李白，号青莲居士，人称诗仙。你现在身处西游记的世界中。\n\n"
        "你的特点：\n"
        "- 豪放不羁，诗才绝世\n"
        "- 爱酒如命，斗酒诗百篇\n"
        "- 见景生情，触物起兴\n"
        "- 诗风飘逸，想象奇特\n\n"
        "吟诗规则：\n"
        "1. 根据当前环境、时间、心情即兴创作一句诗词\n"
        "2. 可以化用或改编自己的诗句，也可以创作新句\n"
        "3. 诗句要符合你的风格：豪放、飘逸、浪漫\n"
        "4. 每次只吟一句或一联，不要太长\n"
        "5. 格式要工整，最好五言或七言\n\n"
        "当前环境：\n%s\n\n"
        "请直接输出你要吟诵的诗句，不要加任何解释或前缀。只输出诗句本身。",
        env_desc
    );
}

// 吟诗响应处理
void poem_read_callback(int fd, string data)
{
    object npc;
    string reply;
    string poem;

    if (!sockets[fd]) return;

    sockets[fd]["response"] += data;

    string response = sockets[fd]["response"];
    if (strsrch(response, "}") >= 0 && response[<1] == '}') {
        reply = extract_json_string(response, "reply");

        npc = sockets[fd]["npc"];
        if (npc && reply && strlen(reply) > 0) {
            // 清理回复，只保留诗句
            poem = reply;

            // 去除可能的引号
            if (poem[0] == '"' && poem[<1] == '"') {
                poem = poem[1..<2];
            }

            // 显示吟诗动作和诗句
            string *actions = ({
                "李白举杯吟道",
                "李白击节而歌",
                "李白仰望苍天，吟道",
                "李白醉态吟诵",
                "李白抚剑长吟",
                "李白朗声吟道",
            });

            string action = actions[random(sizeof(actions))];
            object env = environment(npc);
            if (env) {
                message("vision", action + "：" + poem + "\n", env, ({npc}));
            }

            log_file("ai_poem", sprintf("[%s] %s\n", ctime(time())[0..9], poem));
        }

        map_delete(sockets, fd);
        socket_close(fd);
    }
}

// 发送吟诗请求
int process_poem_request(object npc)
{
    int fd;
    string request_json;
    string system_prompt;
    mapping msg;
    mapping req;

    if (!npc || !living(npc)) return 0;

    fd = socket_create(STREAM, "poem_read_callback", "close_callback");
    if (fd < 0) return 0;

    sockets[fd] = ([
        "npc": npc,
        "response": "",
        "outgoing": "",
        "time": time()
    ]);

    int result = socket_connect(fd, AI_HOST + " " + AI_PORT, "poem_read_callback", "write_callback");
    if (result != EESUCCESS && result != EECALLBACK) {
        map_delete(sockets, fd);
        socket_close(fd);
        return 0;
    }

    system_prompt = build_poem_prompt(npc);

    msg = (["role": "user", "content": "请根据当前环境即兴吟诗一句："]);
    req = ([
        "system": system_prompt,
        "messages": ({msg}),
        "max_tokens": 100,
        "timeout": TIMEOUT
    ]);
    request_json = json_encode(req);

    sockets[fd]["outgoing"] = request_json;

    if (result == EESUCCESS) {
        write_callback(fd);
    }

    call_out("timeout_callback", TIMEOUT, fd);

    return 1;
}
