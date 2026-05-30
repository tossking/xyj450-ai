// 任务使者 - AI智能任务NPC
// 特点：发布智能随机任务，AI对话，自主行动

inherit NPC;

int give_quest(string arg);
int complete_quest();
int cancel_quest();
void ai_think();
int execute_ai_action(string action_type, string action_data);
int do_move(string direction);

// 任务列表 - 根据难度分级（添加地点、理由和目标ID）
mapping *easy_quests = ({
    (["quest": "收破烂的", "quest_id": "shou polan", "quest_type": "杀", "exp_bonus": 50, "silver": 10,
      "location": "长安城东门附近游荡", "reason": "此人偷了我组织的财物，需教训一番"]),
    (["quest": "乞丐", "quest_id": "qigai", "quest_type": "杀", "exp_bonus": 50, "silver": 10,
      "location": "长安城南门大街游荡", "reason": "这乞丐是敌帮的眼线，需除掉"]),
    (["quest": "樵夫", "quest_id": "qiaofu", "quest_type": "杀", "exp_bonus": 60, "silver": 15,
      "location": "长安城外树林", "reason": "他欠我组织一笔债迟迟不还"]),
    (["quest": "农夫", "quest_id": "nongfu", "quest_type": "杀", "exp_bonus": 60, "silver": 15,
      "location": "长安城外农田", "reason": "此人暗中资助我们的敌人"]),
    (["quest": "铁锤", "quest_id": "hammer", "quest_type": "寻", "exp_bonus": 40, "silver": 20,
      "location": "长安城铁匠铺(青龙大街东)", "reason": "这把铁锤是练功之物，帮我买来"]),
    (["quest": "匕首", "quest_id": "dagger", "quest_type": "寻", "exp_bonus": 45, "silver": 25,
      "location": "长安城铁匠铺(青龙大街东)", "reason": "防身利器，帮我取一把"]),
    (["quest": "铁斧", "quest_id": "axe", "quest_type": "寻", "exp_bonus": 45, "silver": 25,
      "location": "长安城铁匠铺(青龙大街东)", "reason": "砍柴用的工具"]),
});

mapping *medium_quests = ({
    (["quest": "店小二", "quest_id": "xiao er", "quest_type": "杀", "exp_bonus": 100, "silver": 50,
      "location": "长安城千金楼或喜福会", "reason": "此人泄露了我方的秘密，需处理"]),
    (["quest": "茶博士", "quest_id": "cha boshi", "quest_type": "杀", "exp_bonus": 100, "silver": 50,
      "location": "长安城茶馆(青云楼)", "reason": "他打听到了不该知道的事"]),
    (["quest": "老裁缝", "quest_id": "lao caifeng", "quest_type": "杀", "exp_bonus": 120, "silver": 60,
      "location": "长安城布铺(南城风云布铺)", "reason": "此人暗中为敌方缝制标识衣物"]),
    (["quest": "磨菜刀的", "quest_id": "mo caidao", "quest_type": "杀", "exp_bonus": 80, "silver": 40,
      "location": "长安城集市游荡", "reason": "他磨的刀伤了我们要员"]),
    (["quest": "卖鱼的", "quest_id": "mai yu", "quest_type": "杀", "exp_bonus": 80, "silver": 40,
      "location": "长安城集市游荡", "reason": "鱼肆恶霸，欺压百姓需惩治"]),
    (["quest": "长剑", "quest_id": "sword", "quest_type": "寻", "exp_bonus": 100, "silver": 80,
      "location": "长安城兵器铺(东城镇风兵器铺)", "reason": "我需要一把趁手的兵器"]),
    (["quest": "铁枪", "quest_id": "spear", "quest_type": "寻", "exp_bonus": 80, "silver": 60,
      "location": "长安城兵器铺(东城镇风兵器铺)", "reason": "组织训练需要长枪"]),
    (["quest": "玉佩", "quest_id": "yupei", "quest_type": "寻", "exp_bonus": 90, "silver": 70,
      "location": "长安城珠宝行(朱雀大街)", "reason": "要送人的贵重礼物"]),
});

mapping *hard_quests = ({
    (["quest": "张铁匠", "quest_id": "zhang tiejiang", "quest_type": "杀", "exp_bonus": 200, "silver": 100,
      "location": "长安城铁匠铺(西城)", "reason": "他打造的兵器流入敌手，需调查"]),
    (["quest": "陕北大汉", "quest_id": "dahan", "quest_type": "杀", "exp_bonus": 180, "silver": 90,
      "location": "长安城酒楼或客栈游荡", "reason": "此人欠债不还，需教训一番"]),
    (["quest": "王仁德", "quest_id": "kao guan", "quest_type": "杀", "exp_bonus": 250, "silver": 120,
      "location": "长安城贡院(北城及第街)", "reason": "此人泄露科举试题，证据在他身上"]),
    (["quest": "龙小云", "quest_id": "long xiaoyun", "quest_type": "杀", "exp_bonus": 300, "silver": 150,
      "location": "长安城龙府", "reason": "龙家少主知晓太多秘密，必须除掉"]),
    (["quest": "牛皮盾", "quest_id": "shield", "quest_type": "寻", "exp_bonus": 200, "silver": 150,
      "location": "长安城兵器铺(东城镇风兵器铺)", "reason": "上等护具，组织急需"]),
    (["quest": "铜锤", "quest_id": "hammer", "quest_type": "寻", "exp_bonus": 180, "silver": 130,
      "location": "长安城兵器铺(东城镇风兵器铺)", "reason": "重型武器，训练用"]),
    (["quest": "项圈", "quest_id": "xiangquan", "quest_type": "寻", "exp_bonus": 150, "silver": 100,
      "location": "长安城珠宝行(朱雀大街)", "reason": "组织女弟子需要的饰品"]),
});

void create()
{
    set_name("任务使者", ({"renwu shizhe", "renwu", "shizhe", "tasker"}));
    set("long", "一位身穿灰色长袍的中年人，眼神锐利，举止干练。\n他负责给江湖中人分配任务，传闻他背后有神秘势力支持。\n");

    set("gender", "男性");
    set("age", 40);
    set("attitude", "peaceful");

    set("str", 25);
    set("int", 30);
    set("con", 25);
    set("dex", 25);

    set("max_kee", 3000);
    set("kee", 3000);
    set("max_sen", 2000);
    set("sen", 2000);
    set("combat_exp", 500000);

    // AI NPC 配置
    set("ai_enabled", 1);
    set("ai_autonomous", 1);
    set("ai_action_interval", 600);

    set("ai_personality",
        "你是任务使者，负责给江湖中人分配任务。\n\n"
        "性格特点：\n"
        "- 干练果断，说话简洁有力\n"
        "- 对完成任务的人给予丰厚奖励\n"
        "- 不喜欢懒惰和拖延的人\n"
        "- 会根据人的实力分配适当难度的任务\n"
        "- 偶尔会透露一些江湖消息\n\n"
        "说话风格：\n"
        "- 简洁干练，不喜欢废话\n"
        "- 会用「好」「不错」「继续努力」等短句\n"
        "- 对新人会比较耐心解释\n"
    );

    set_skill("unarmed", 100);
    set_skill("dodge", 100);
    set_skill("parry", 100);

    setup();
    set_heart_beat(1);
    call_out("ai_think", 30);
}

void init()
{
    ::init();
    add_action("give_quest", "task");
    add_action("complete_quest", "taskdone");
    add_action("cancel_quest", "taskcancel");
    add_action("wiz_task_complete", "taskcomplete");  // 巫师测试命令
}

// 巫师测试命令：直接完成任务
int wiz_task_complete(string arg)
{
    object me = this_player();

    if (!wizardp(me)) {
        command("say 你没有权限使用这个命令。");
        return 1;
    }

    if (!me->query("task/pending")) {
        command("say 你没有领取任务啊。");
        return 1;
    }

    // 标记任务完成
    me->set("task/kill_done", 1);
    me->set("task/item_found", 1);

    command("say 好的，任务已标记完成。");
    return 1;
}

// 评估玩家实力
int evaluate_player(object player)
{
    int exp;

    if (!player) return 0;

    exp = player->query("combat_exp");

    if (exp < 10000) return 0;      // 新手
    if (exp < 100000) return 1;     // 初级
    if (exp < 500000) return 2;     // 中级
    return 3;                       // 高级
}

// 发布任务
int give_quest(string arg)
{
    object me = this_player();
    object ob = this_object();
    mapping quest;
    int level;
    int time_limit;
    mapping *quest_list;

    if (me->is_fighting()) {
        command("say 你正忙着呢，等打完再说。");
        return 1;
    }

    // 检查是否已有任务
    if (me->query("task/pending")) {
        command("say 你不是已经有任务了吗？先去完成再说。");
        command("say 你现在的任务是：" + me->query("task/pending/quest_type") +
                "『" + me->query("task/pending/quest") + "』");
        return 1;
    }

    // 根据玩家实力选择任务难度
    level = evaluate_player(me);

    switch (level) {
        case 0:
            quest_list = easy_quests;
            time_limit = 600;  // 10分钟
            command("say 新来的？给你安排个简单的任务练练手。");
            break;
        case 1:
            quest_list = easy_quests + medium_quests;
            time_limit = 480;  // 8分钟
            command("say 不错，有点基础了。这个任务应该难不倒你。");
            break;
        case 2:
            quest_list = medium_quests + hard_quests;
            time_limit = 360;  // 6分钟
            command("say 你的实力不错，这个任务需要点真本事。");
            break;
        default:
            quest_list = hard_quests;
            time_limit = 300;  // 5分钟
            command("say 高手！给你个有挑战性的任务。");
            break;
    }

    // 随机选择任务
    quest = quest_list[random(sizeof(quest_list))];

    // 记录任务
    me->set("task/pending", quest);
    me->set("task/start_time", time());
    me->set("task/time_limit", time_limit);
    me->set("task/employer", "任务使者");

    // 发布任务 - 显示详细信息
    tell_object(me, "\n═══════════════════════════════════════\n");
    if (quest["quest_type"] == "杀") {
        if (quest["quest_id"]) {
            tell_object(me, sprintf("【任务】除掉『%s』(%s)\n", quest["quest"], quest["quest_id"]));
        } else {
            tell_object(me, sprintf("【任务】除掉『%s』\n", quest["quest"]));
        }
    } else {
        if (quest["quest_id"]) {
            tell_object(me, sprintf("【任务】寻找『%s』(%s)\n", quest["quest"], quest["quest_id"]));
        } else {
            tell_object(me, sprintf("【任务】寻找『%s』\n", quest["quest"]));
        }
    }

    // 显示地点
    if (quest["location"]) {
        tell_object(me, sprintf("【地点】%s\n", quest["location"]));
    }

    // 显示理由
    if (quest["reason"]) {
        tell_object(me, sprintf("【原因】%s\n", quest["reason"]));
    }

    tell_object(me, sprintf("【时限】%d秒\n", time_limit));
    tell_object(me, sprintf("【奖励】%d实战经验，%d两银子\n", quest["exp_bonus"], quest["silver"]));
    tell_object(me, "═══════════════════════════════════════\n");

    // NPC口头说明
    if (quest["quest_type"] == "杀") {
        command("say 去『" + quest["quest"] + "』那里处理一下。");
    } else {
        command("say 帮我把『" + quest["quest"] + "』找回来。");
    }

    if (quest["location"]) {
        command("say 你可以去" + quest["location"] + "找找。");
    }

    return 1;
}

// 完成任务
int complete_quest()
{
    object me = this_player();
    mapping task;
    int time_used;
    int exp_bonus;
    int silver;
    object money;
    int start_time;
    int time_limit;

    if (!me->query("task/pending")) {
        command("say 你没有领取任务啊。");
        return 1;
    }

    task = me->query("task/pending");
    start_time = me->query("task/start_time");
    time_limit = me->query("task/time_limit");

    // 处理时间计算
    if (start_time && intp(start_time)) {
        time_used = time() - start_time;
    } else {
        time_used = 0;
    }

    // 检查是否超时
    if (time_limit && intp(time_limit) && time_used > time_limit) {
        command("say 太慢了！任务已经超时了！");
        command("say 这次就算了，下次快点！");
        me->delete("task");
        return 1;
    }

    // 验证任务完成
    if (task["quest_type"] == "杀") {
        // 杀怪任务需要验证
        if (!me->query("task/kill_done")) {
            command("say 你还没完成任务呢！");
            if (task["location"]) {
                if (task["quest_id"]) {
                    command("say 去" + task["location"] + "找『" + task["quest"] + "』(" + task["quest_id"] + ")。");
                } else {
                    command("say 去" + task["location"] + "找『" + task["quest"] + "』。");
                }
            } else {
                command("say 去杀了『" + task["quest"] + "』再来。");
            }
            return 1;
        }
    } else if (task["quest_type"] == "寻") {
        // 寻物任务需要验证
        if (!me->query("task/item_found")) {
            command("say 你还没找到『" + task["quest"] + "』呢！");
            if (task["location"]) {
                command("say 去" + task["location"] + "找找看。");
            }
            return 1;
        }
    }

    // 计算奖励（提前完成有额外奖励）
    exp_bonus = task["exp_bonus"];
    silver = task["silver"];

    if (time_used < me->query("task/time_limit") / 2) {
        // 一半时间内完成，奖励翻倍
        exp_bonus = exp_bonus * 2;
        silver = silver * 2;
        command("say 干得漂亮！提前完成，奖励翻倍！");
    } else if (time_used < me->query("task/time_limit") * 3 / 4) {
        // 75%时间内完成，奖励加半
        exp_bonus = exp_bonus * 3 / 2;
        silver = silver * 3 / 2;
        command("say 不错不错，效率挺高！");
    } else {
        command("say 还行，勉强按时完成。");
    }

    // 发放奖励
    me->add("combat_exp", exp_bonus);
    tell_object(me, sprintf("你获得了%d点实战经验！\n", exp_bonus));

    if (silver > 0) {
        money = new("/obj/money/silver");
        money->set_amount(silver);
        if (money->move(me)) {
            tell_object(me, sprintf("任务使者给了你%d两银子！\n", silver));
        } else {
            destruct(money);
            tell_object(me, "你身上太重了，拿不动银子。\n");
        }
    }

    // 记录完成次数（使用独立的统计字段）
    me->add("task_stats/completed", 1);
    me->add("task_stats/total_exp", exp_bonus);
    me->add("task_stats/total_silver", silver);

    // 清除当前任务
    me->delete("task/pending");
    me->delete("task/start_time");
    me->delete("task/time_limit");
    me->delete("task/employer");
    me->delete("task/kill_done");
    me->delete("task/item_found");

    command("say 继续努力！还有更多任务等着你。");

    return 1;
}

// 取消任务
int cancel_quest()
{
    object me = this_player();

    if (!me->query("task/pending")) {
        command("say 你没有领取任务啊。");
        return 1;
    }

    command("say 不想做了？行吧，这次不扣你什么。");
    me->delete("task");
    command("say 需要任务再来找我。");

    return 1;
}

// 查看任务状态
int query_task_status(object player)
{
    mapping task;
    int time_left;

    if (!player) return 0;

    task = player->query("task/pending");
    if (!task) return 0;

    time_left = player->query("task/time_limit") - (time() - player->query("task/start_time"));

    if (time_left <= 0) {
        return -1;  // 超时
    }

    return time_left;
}

// AI思考
void ai_think()
{
    object me = this_object();
    object ai_d;

    if (!me || !living(me)) return;
    if (me->is_fighting()) {
        call_out("ai_think", 45);
        return;
    }

    if (me->query("ai_last_action") &&
        time() - me->query("ai_last_action") < me->query("ai_action_interval")) {
        call_out("ai_think", 10);
        return;
    }

    ai_d = load_object("/adm/daemons/ai_clientd");
    if (ai_d) {
        ai_d->process_autonomous_action(me);
    }

    me->set("ai_last_action", time());
    call_out("ai_think", me->query("ai_action_interval"));
}

// 执行AI行动
int execute_ai_action(string action_type, string action_data)
{
    object me = this_object();
    if (!living(me)) return 0;

    switch (action_type) {
        case "说话":
        case "say":
            command("say " + action_data);
            return 1;
        case "移动":
        case "move":
            return do_move(action_data);
        case "发布任务":
        case "quest":
            // 向周围的玩家发布任务提示
            command("say 有人要接任务吗？来找我！");
            return 1;
        case "休息":
        case "rest":
            command("exercise");
            return 1;
    }
    return 0;
}

// 移动
int do_move(string direction)
{
    object me = this_object();
    object env = environment(me);
    mapping exits;
    string *dirs;

    if (!env) return 0;

    exits = env->query("exits");
    if (!exits || !mapp(exits)) {
        return 0;
    }

    dirs = keys(exits);
    if (!exits[direction]) {
        direction = dirs[random(sizeof(dirs))];
    }

    command("go " + direction);
    return 1;
}

// 接受对话
int accept_object(object who, object ob)
{
    string ob_name;
    mapping task;
    string quest_name;

    if (!who || !ob) return 0;

    task = who->query("task/pending");
    if (!task || !mapp(task)) {
        command("say 给我东西做什么？我没任务给你。");
        return 0;
    }

    ob_name = ob->query("name");
    quest_name = task["quest"];

    // 检查是否是任务物品
    if (task["quest_type"] == "寻" && quest_name &&
        strsrch(ob_name, quest_name) >= 0) {
        command("say 好的，找到了！");
        destruct(ob);
        // 自动完成任务
        who->set("task/item_found", 1);
        complete_quest();
        return 1;
    }

    command("say 这个不是我要的东西。");
    return 0;
}

// 无法被杀死
void die()
{
    object me = this_object();

    command("say 何必呢？我只是个发布任务的。");

    set("kee", query("max_kee"));
    set("eff_kee", query("max_kee"));
    set("sen", query("max_sen"));
    set("eff_sen", query("max_sen"));

    remove_all_killer();
}

void unconcious()
{
    set("kee", query("max_kee"));
    set("eff_kee", query("max_kee"));
    set("sen", query("max_sen"));
    set("eff_sen", query("max_sen"));
}

int accept_fight(object who)
{
    command("say 我不和人打架，只发布任务。");
    return 0;
}

int accept_kill(object who)
{
    command("say 杀我没用的，我只是个任务使者。");
    return 0;
}
