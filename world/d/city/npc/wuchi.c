// 武痴 - AI自主行动NPC
// 特点：热爱武术，主动找人切磋，会移动、说话、给钱、打架

inherit NPC;

void ai_think();
int execute_ai_action(string action_type, string action_data);
int do_move(string direction);
int do_challenge(string target_id);
void win_combat(object opponent);
void lose_combat(object opponent);
string evaluate_opponent(object opponent);

void create()
{
    set_name("武痴", ({"wuchi", "martial", "fanatic"}));
    set("long", "一个身材魁梧的中年男子，浑身肌肉虬结，眼神中透着对武学的狂热。\n他不停地活动着手脚，看起来随时准备找人切磋武艺。\n");

    set("gender", "男性");
    set("age", 35);
    set("attitude", "peaceful");

    set("str", 35);
    set("int", 18);
    set("con", 30);
    set("dex", 32);

    set("max_kee", 5000);
    set("kee", 5000);
    set("max_sen", 3000);
    set("sen", 3000);
    set("max_force", 3000);
    set("force", 3000);
    set("combat_exp", 1000000);

    // AI NPC 配置
    set("ai_enabled", 1);
    set("ai_autonomous", 1);
    set("ai_action_interval", 600);  // 30秒思考一次

    set("ai_personality",
        "你是武痴，长安城内赫赫有名的武术狂人。\n\n"
        "性格特点：\n"
        "- 对武术极其痴迷，见到人就想去切磋\n"
        "- 性格直爽豪迈，说话粗犷不拘小节\n"
        "- 尊重强者，鄙视弱者，但愿意指点新人\n"
        "- 输赢都坦然接受，从不记仇\n"
        "- 有钱任性，喜欢用银子诱惑人打架\n\n"
        "说话风格：\n"
        "- 粗犷豪迈，喜欢说「哈哈」「嘿嘿」\n"
        "- 称呼别人为「小子」「朋友」「好汉」\n"
        "- 见到高手会抱拳行礼，见到新手会挑衅\n"
    );

    set("ai_actions", ({"say", "move", "challenge", "rest"}));
    set("combat_reward", 500);
    set("combat_cooldown", 180);

    set_skill("unarmed", 150);
    set_skill("dodge", 150);
    set_skill("parry", 150);
    set_skill("force", 150);
    set_skill("cuff", 150);

    map_skill("unarmed", "cuff");
    map_skill("parry", "cuff");

    setup();
    set_heart_beat(1);
    call_out("ai_think", 30);
}

// 心跳检测
void heart_beat()
{
    object me = this_object();
    object opponent;
    int kee, max_kee;
    float hp_ratio;

    if (!me) return;

    kee = query("kee");
    max_kee = query("max_kee");
    hp_ratio = to_float(kee) / to_float(max_kee);

    // 检查HP，如果为负触发失败处理
    if (kee < 0 || query("sen") < 0) {
        opponent = query_temp("last_opponent");
        if (opponent && environment(opponent) == environment(me)) {
            lose_combat(opponent);
        } else {
            command("say 呼...打得真过瘾！");
        }

        // 恢复状态
        set("kee", max_kee);
        set("eff_kee", max_kee);
        set("sen", query("max_sen"));
        set("eff_sen", query("max_sen"));

        remove_all_killer();
        set("last_combat_time", time());
        delete_temp("last_opponent");
        delete("challenging");
        return;
    }

    if (!living(me)) return;

    // 检查战斗状态 - 血量低于20%时主动认输
    if (me->is_fighting() && hp_ratio < 0.2 && !query_temp("surrendering")) {
        opponent = query_temp("last_opponent");
        if (opponent && living(opponent) && environment(opponent) == environment(me)) {
            set_temp("surrendering", 1);  // 防止重复触发

            command("say 好了好了！我认输！你的武功真厉害！");
            remove_all_killer();
            opponent->remove_killer(me);

            // 触发失败处理（给奖励等）
            lose_combat(opponent);

            // 恢复状态
            set("kee", max_kee);
            set("eff_kee", max_kee);
            set("sen", query("max_sen"));
            set("eff_sen", query("max_sen"));

            set("last_combat_time", time());
            delete_temp("last_opponent");
            delete("challenging");
            delete_temp("surrendering");
            return;
        }
    }

    // 检查战斗状态
    if (me->is_fighting()) {
        opponent = me->query_temp("last_opponent");
        if (!opponent || !living(opponent) || environment(opponent) != environment(me)) {
            remove_all_killer();
            if (opponent) opponent->remove_killer(me);
            delete_temp("last_opponent");
            delete("challenging");
            command("say 咦？人呢？跑得倒挺快！");
        }
    }
}

// 评估对手实力
string evaluate_opponent(object opponent)
{
    int exp, my_exp;

    if (!opponent) return "未知";

    exp = opponent->query("combat_exp");
    my_exp = query("combat_exp");

    if (exp > my_exp * 2) return "绝世高手";
    if (exp > my_exp * 1.5) return "顶尖高手";
    if (exp > my_exp) return "高手";
    if (exp > my_exp * 0.7) return "势均力敌";
    if (exp > my_exp * 0.4) return "略有差距";
    if (exp > my_exp * 0.2) return "新手";
    return "初出茅庐";
}

// AI思考
void ai_think()
{
    object me = this_object();
    object ai_d;

    if (!me || !living(me)) return;
    if (me->is_fighting()) {
        call_out("ai_think", 30);
        return;
    }

    if (me->query("ai_last_action") &&
        time() - me->query("ai_last_action") < me->query("ai_action_interval")) {
        call_out("ai_think", 5);
        return;
    }

    if (me->query("last_combat_time") &&
        time() - me->query("last_combat_time") < me->query("combat_cooldown")) {
        if (random(3) == 0) {
            command("say 呼...刚才那架打得真过瘾！");
        }
        call_out("ai_think", 30);
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
        case "挑战":
        case "challenge":
            return do_challenge(action_data);
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
        command("say 这地方怎么没路了？");
        return 0;
    }

    dirs = keys(exits);
    if (!exits[direction]) {
        direction = dirs[random(sizeof(dirs))];
    }

    command("go " + direction);
    return 1;
}

// 挑战
int do_challenge(string target_id)
{
    object me = this_object();
    object env = environment(me);
    object target = 0;
    int reward;
    object *inv;
    int i;
    string level;

    if (!env) return 0;

    if (me->is_fighting()) {
        command("say 我正在打呢！");
        return 0;
    }

    if (me->query("last_combat_time") &&
        time() - me->query("last_combat_time") < me->query("combat_cooldown")) {
        command("say 让我歇会儿...");
        return 0;
    }

    // 找玩家目标
    inv = all_inventory(env);
    for (i = 0; i < sizeof(inv); i++) {
        if (living(inv[i]) && inv[i] != me && userp(inv[i])) {
            target = inv[i];
            break;
        }
    }

    // 找其他生物
    if (!target) {
        for (i = 0; i < sizeof(inv); i++) {
            if (living(inv[i]) && inv[i] != me) {
                target = inv[i];
                break;
            }
        }
    }

    if (!target) {
        command("say 怎么连个能打的都没有？");
        return 0;
    }

    // 评估对手实力
    level = evaluate_opponent(target);
    reward = query("combat_reward");

    // 根据对手实力调整
    if (level == "绝世高手" || level == "顶尖高手") {
        reward = 1000;
        command("say 哇！" + target->query("name") + "！久仰大名！彩头翻倍！");
    } else if (level == "高手") {
        reward = 700;
        command("say 嘿！" + target->query("name") + "！看你是个练家子！");
    } else if (level == "新手" || level == "初出茅庐") {
        reward = 200;
        command("say 小子！我看你骨骼不错，来练练？");
    } else {
        command("say " + target->query("name") + "！我看你骨骼惊奇，来打一架吧！");
    }

    command("say 我出" + chinese_number(reward) + "两银子做彩头！");

    set("challenging", target);
    set_temp("last_opponent", target);
    target->set_temp("challenged_by", me);
    target->set_temp("challenge_reward", reward);

    me->kill_ob(target);
    target->kill_ob(me);

    return 1;
}

// 胜利
void win_combat(object opponent)
{
    string level;

    if (!opponent) return;

    level = evaluate_opponent(opponent);

    if (level == "绝世高手" || level == "顶尖高手") {
        command("say 哈哈！居然赢了！今天运气真好！");
    } else if (level == "新手" || level == "初出茅庐") {
        command("say 小子，还要多练练啊！");
    } else {
        command("say 哈哈！痛快痛快！你的武功不错嘛！");
    }

    add("combat_exp", 1000 + random(500));
    if (userp(opponent)) {
        opponent->add("combat_exp", 500);
        tell_object(opponent, "虽然输了，你还是获得了500点实战经验！\n");
    }

    set("last_combat_time", time());
    delete("challenging");
}

// 失败
void lose_combat(object opponent)
{
    int reward;
    object money;
    string level;

    if (!opponent) {
        command("say 呼...打得真过瘾！");
        set("last_combat_time", time());
        return;
    }

    level = evaluate_opponent(opponent);

    command("say 好！好武功！我输了！");

    if (userp(opponent)) {
        reward = query_temp("challenge_reward");
        if (!reward || reward < 1) reward = query("combat_reward");

        if (reward > 0) {
            money = new("/obj/money/silver");
            money->set_amount(reward);
            if (money->move(opponent)) {
                command("say 给，这是说好的" + chinese_number(reward) + "两银子！");
                tell_object(opponent, "武痴给了你" + chinese_number(reward) + "两银子！\n");
            } else {
                destruct(money);
                command("say 哎呀，你拿不动了！");
            }
        }

        if (level == "绝世高手" || level == "顶尖高手") {
            opponent->add("combat_exp", 2000);
            tell_object(opponent, "战胜了高手武痴，你获得了2000点实战经验！\n");
        } else {
            opponent->add("combat_exp", 1000);
            tell_object(opponent, "战胜了武痴，你获得了1000点实战经验！\n");
        }
    }

    add("combat_exp", 500);
    set("last_combat_time", time());
    delete("challenging");
    delete_temp("challenge_reward");
}

void die()
{
    object opponent = query_temp("last_opponent");

    remove_all_killer();
    if (opponent) {
        opponent->remove_killer(this_object());
        lose_combat(opponent);
    }

    command("say 呼...这次输得心服口服！好武功！");

    set("kee", query("max_kee"));
    set("eff_kee", query("max_kee"));
    set("sen", query("max_sen"));
    set("eff_sen", query("max_sen"));

    set("last_combat_time", time());
    delete_temp("last_opponent");
    delete("challenging");
}

void unconcious()
{
    object opponent = query_temp("last_opponent");

    remove_all_killer();
    if (opponent) {
        opponent->remove_killer(this_object());
        lose_combat(opponent);
    }

    command("say 等等...让我缓口气...");

    set("kee", query("max_kee") / 2);
    set("eff_kee", query("max_kee") / 2);
    set("sen", query("max_sen") / 2);
    set("eff_sen", query("max_sen") / 2);

    set("last_combat_time", time());
    delete_temp("last_opponent");
    delete("challenging");
}

int accept_fight(object who)
{
    string level = evaluate_opponent(who);

    command("say 好！来吧！");

    if (level == "绝世高手" || level == "顶尖高手") {
        command("say 能与高手过招，是我的荣幸！");
    }

    set("challenging", who);
    set_temp("last_opponent", who);
    return 1;
}

int accept_kill(object who)
{
    command("say 想杀我？来试试！正合我意！");
    set("challenging", who);
    set_temp("last_opponent", who);
    return 1;
}

void fight_ob(object ob)
{
    if (ob && ob != this_object()) {
        set_temp("last_opponent", ob);
    }
    ::fight_ob(ob);
}

void kill_ob(object ob)
{
    if (ob && ob != this_object()) {
        set_temp("last_opponent", ob);
    }
    ::kill_ob(ob);
}
