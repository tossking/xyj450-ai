// AI Client Daemon - 与AI服务器通信
// 西游记MUD AI NPC系统

#include <ansi.h>
#include <net/socket.h>

// 配置
#define AI_HOST "127.0.0.1"
#define AI_PORT 9999
#define TIMEOUT 30

// 全局变量
nosave mapping npc_contexts;  // NPC上下文记忆
nosave mapping sockets;       // socket信息

void create()
{
    seteuid(getuid());
    npc_contexts = ([]);
    sockets = ([]);
}

// 获取NPC上下文
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

// 添加对话历史
void add_to_history(object npc, string role, string content)
{
    string npc_id = base_name(npc);
    mapping ctx = get_npc_context(npc);
    mixed *history = ctx["history"];

    history += ({ (["role": role, "content": content]) });

    // 保留最近20轮对话
    if (sizeof(history) > 40) {
        history = history[<40..];
    }

    ctx["history"] = history;
    npc_contexts[npc_id] = ctx;
}

// 构建系统提示
string build_system_prompt(object npc)
{
    mapping ctx = get_npc_context(npc);
    string personality = ctx["personality"];
    string name = npc->query("name");

    return sprintf(
        "你是西游记MUD游戏中的NPC「%s」。\n\n"
        "人物设定：\n%s\n\n"
        "规则：\n"
        "1. 用中文回复，语气符合人物设定\n"
        "2. 回复简短自然，像游戏对话(1-3句话)\n"
        "3. 可以在回复开头用【动作】格式添加动作，如【点头】\n"
        "4. 保持角色一致性",
        name, personality
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
    // 查找字段名
    string search = "\"" + field + "\"";
    int pos = strsrch(json, search);
    if (pos < 0) return 0;

    // 从字段名后找到冒号
    int colon_pos = strsrch(json[pos..], ":");
    if (colon_pos < 0) return 0;
    colon_pos += pos;

    // 找到引号开始
    int start = colon_pos + 1;
    while (start < strlen(json) && (json[start] == ' ' || json[start] == '\t')) {
        start++;
    }
    if (start >= strlen(json) || json[start] != '"') return 0;
    start++;  // 跳过开头的引号

    // 找到引号结束
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

// Socket写回调 - 连接建立后发送数据
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
    log_file("ai_client", sprintf("*** read_callback called! fd=%d, data_len=%d ***\n", fd, strlen(data || "")));

    if (!sockets[fd]) {
        log_file("ai_client", "read_callback: sockets[fd] is null!\n");
        return;
    }

    sockets[fd]["response"] += data;

    string response = sockets[fd]["response"];
    log_file("ai_client", sprintf("Total response: %d bytes, content: %s\n", strlen(response), response));

    if (strsrch(response, "}") >= 0 && response[<1] == '}') {
        string reply = extract_json_string(response, "reply");
        log_file("ai_client", sprintf("Parsed reply: %s\n", reply ? reply : "null"));

        if (reply && strlen(reply) > 0) {
            object npc = sockets[fd]["npc"];
            object player = sockets[fd]["player"];
            string input = sockets[fd]["input"];

            log_file("ai_client", sprintf("npc=%O, player=%O\n", npc ? "exists" : "null", player ? "exists" : "null"));

            if (npc && player) {
                log_file("ai_client", sprintf("npc_env=%O, player_env=%O\n", environment(npc), environment(player)));

                string action = "";
                string reply_text = reply;

                int action_start = strsrch(reply, "【");
                int action_end = strsrch(reply, "】");
                if (action_start >= 0 && action_end > action_start) {
                    // LPC字符串索引是字符位置，不是字节位置
                    action = reply[action_start + 1 .. action_end - 1];
                    reply_text = reply[action_end + 1..];

                    if (strlen(action) > 0 && npc) {
                        npc->command(action);
                    }
                }

                add_to_history(npc, "user", input);
                add_to_history(npc, "assistant", reply);

                if (strlen(reply_text) > 0) {
                    log_file("ai_client", sprintf("Calling message_vision with reply_text: %s\n", reply_text));
                    message_vision(CYN "$N说道：" + reply_text + "\n" NOR, npc);
                    log_file("ai_client", "message_vision called\n");
                }
            } else {
                log_file("ai_client", "npc or player is null, skipping message\n");
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
        log_file("ai_client", sprintf("close_callback response len=%d\n", strlen(resp || "")));
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
    mapping ctx;
    mixed *messages;
    mixed *history;
    int i;

    fd = socket_create(STREAM, "read_callback", "close_callback");
    if (fd < 0) {
        log_file("ai_client", sprintf("Failed to create socket: %d\n", fd));
        return 0;
    }

    log_file("ai_client", sprintf("Socket created: fd=%d\n", fd));

    // 先设置socket信息
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

    system_prompt = build_system_prompt(npc);
    ctx = get_npc_context(npc);
    history = ctx["history"];

    messages = ({});
    for (i = 0; i < sizeof(history); i++) {
        messages += ({ history[i] });
    }
    messages += ({ (["role": "user", "content": input]) });

    request_json = json_encode(([
        "system": system_prompt,
        "messages": messages,
        "max_tokens": 500,
        "timeout": TIMEOUT
    ]));

    sockets[fd]["outgoing"] = request_json;
    log_file("ai_client", sprintf("Request ready, len=%d\n", strlen(request_json)));

    // 如果连接立即可写，直接发送
    if (result == EESUCCESS) {
        write_callback(fd);
    }

    call_out("timeout_callback", TIMEOUT, fd);

    return 1;
}

void timeout_callback(int fd)
{
    log_file("ai_client", sprintf("timeout_callback fd=%d, sockets[fd]=%O\n", fd, sockets[fd] ? "exists" : "null"));
    if (sockets[fd]) {
        object npc = sockets[fd]["npc"];
        string resp = sockets[fd]["response"];

        log_file("ai_client", sprintf("response len=%d\n", strlen(resp || "")));

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
    string player_name = player->query("name");
    string input = sprintf("%s问：%s", player_name, topic);

    message_vision(CYN "$N若有所思...\n" NOR, npc, player);

    return send_ai_request(npc, player, input);
}

// 清除NPC记忆
int clear_npc_memory(object npc)
{
    string npc_id = base_name(npc);
    map_delete(npc_contexts, npc_id);
    return 1;
}

// 构建自主行动的系统提示
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

    // 获取房间信息
    if (env) {
        room_desc = env->query("short") || "";
        inv = all_inventory(env);
        people = ({});
        players = ({});

        // 先收集玩家（优先级更高）
        for (i = 0; i < sizeof(inv); i++) {
            if (living(inv[i]) && inv[i] != npc && userp(inv[i])) {
                players += ({ inv[i] });
            }
        }

        // 再收集其他生物
        for (i = 0; i < sizeof(inv); i++) {
            if (living(inv[i]) && inv[i] != npc && !userp(inv[i])) {
                people += ({ inv[i] });
            }
        }

        // 合并列表，玩家在前
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

    // 根据NPC类型构建不同的提示
    string npc_id = npc->query("id");
    string action_prompt = "";

    // 检查NPC类型
    if (npc_id == "renwu" || npc_id == "shizhe" || npc_id == "tasker" ||
        strsrch(personality, "任务") >= 0 || strsrch(personality, "发布任务") >= 0) {
        // 任务使者类型的NPC
        action_prompt = "重要：你是任务使者，你的主要目的是发布任务和提醒玩家！\n"
            "- 如果周围有玩家，使用【说话】提醒他们可以接任务\n"
            "- 偶尔使用【发布任务】来广播任务提示\n"
            "- 不要主动移动太远\n\n"
            "行动类型：\n"
            "【说话】内容 - 主动说一句话\n"
            "【发布任务】 - 提醒玩家来接任务\n"
            "【移动】方向 - 移动(north/south/east/west等)\n"
            "【休息】 - 原地休息\n";
    } else {
        // 默认武痴类型的NPC
        action_prompt = "重要：你是武痴，你的主要目的是找人切磋武艺！\n"
            "- 如果周围有人，优先使用【挑战】发起战斗\n"
            "- 【挑战】的格式：目标玩家的英文ID（如god、li bai等）\n"
            "- 只有在没有人时才使用【说话】或【移动】\n\n"
            "行动类型：\n"
            "【挑战】目标ID - 向目标发起挑战并开始战斗\n"
            "【说话】内容 - 主动说一句话\n"
            "【移动】方向 - 移动(north/south/east/west等)\n"
            "【休息】 - 原地休息\n";
    }

    return sprintf(
        "你是西游记MUD游戏中的NPC「%s」。\n\n"
        "人物设定：\n%s\n\n"
        "当前环境：\n"
        "房间：%s\n"
        "%s\n\n"
        "请决定你接下来要做什么，只能做一个行动。\n"
        "返回格式（只返回这一行）：\n"
        "【行动类型】行动数据\n\n"
        "%s\n"
        "注意：只返回一行行动指令，不要有多余内容。",
        name, personality, room_desc, people_desc, action_prompt
    );
}

// 处理自主行动响应
void process_autonomous_response(int fd, string response)
{
    string reply = extract_json_string(response, "reply");
    string action_type, action_data;
    object npc, player;

    if (!reply || strlen(reply) == 0) {
        log_file("ai_client", "autonomous_response: empty reply\n");
        return;
    }

    log_file("ai_client", sprintf("autonomous reply: %s\n", reply));

    npc = sockets[fd]["npc"];
    if (!npc) return;

    // 解析行动类型和数据
    int action_start = strsrch(reply, "【");
    int action_end = strsrch(reply, "】");

    if (action_start >= 0 && action_end > action_start) {
        action_type = reply[action_start + 1 .. action_end - 1];
        action_data = reply[action_end + 1..];

        // 清理多余空格
        action_data = replace_string(action_data, "  ", " ");
        action_data = replace_string(action_data, " ", " ");

        log_file("ai_client", sprintf("action_type=%s, action_data=%s\n", action_type, action_data));

        // 执行行动
        if (npc->execute_ai_action(action_type, action_data)) {
            log_file("ai_client", "action executed successfully\n");
        } else {
            log_file("ai_client", "action execution failed\n");
        }
    }
}

// 自主行动的读回调
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

// 发送自主行动请求
int process_autonomous_action(object npc)
{
    int fd;
    string request_json;
    string system_prompt;
    mapping msg;
    mapping req;

    if (!npc || !living(npc)) return 0;

    fd = socket_create(STREAM, "autonomous_read_callback", "close_callback");
    if (fd < 0) {
        log_file("ai_client", sprintf("autonomous: Failed to create socket: %d\n", fd));
        return 0;
    }

    sockets[fd] = ([
        "npc": npc,
        "response": "",
        "outgoing": "",
        "time": time()
    ]);

    int result = socket_connect(fd, AI_HOST + " " + AI_PORT, "autonomous_read_callback", "write_callback");
    if (result != EESUCCESS && result != EECALLBACK) {
        log_file("ai_client", sprintf("autonomous: Failed to connect: %d\n", result));
        map_delete(sockets, fd);
        socket_close(fd);
        return 0;
    }

    system_prompt = build_autonomous_prompt(npc);

    // 创建消息映射(不能在数组内直接创建mapping)
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
