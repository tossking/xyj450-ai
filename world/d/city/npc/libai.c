// 李白 - AI诗仙NPC
// 特点：爱酒如命，诗才绝世，剑术通神，豪放不羁
// 送酒可以增加好感度

#include <ansi.h>

inherit NPC;

void ai_think();
int accept_gift(object who, object ob);
int do_give_wine(object who, object ob);
int recite_poem(string theme);
string get_drunk_message();

// 李白诗集
string *poems_drinking = ({
    "花间一壶酒，独酌无相亲。举杯邀明月，对影成三人。",
    "人生得意须尽欢，莫使金樽空对月。天生我材必有用，千金散尽还复来。",
    "兰陵美酒郁金香，玉碗盛来琥珀光。但使主人能醉客，不知何处是他乡。",
    "两人对酌山花开，一杯一杯复一杯。我醉欲眠卿且去，明朝有意抱琴来。",
});

string *poems_moon = ({
    "床前明月光，疑是地上霜。举头望明月，低头思故乡。",
    "青天有月来几时？我今停杯一问之。人攀明月不可得，月行却与人相随。",
});

string *poems_hero = ({
    "赵客缦胡缨，吴钩霜雪明。银鞍照白马，飒沓如流星。十步杀一人，千里不留行。",
    "君不见黄河之水天上来，奔流到海不复回。君不见高堂明镜悲白发，朝如青丝暮成雪。",
    "长风破浪会有时，直挂云帆济沧海！",
    "仰天大笑出门去，我辈岂是蓬蒿人！",
});

string *poems_nature = ({
    "飞流直下三千尺，疑是银河落九天。",
    "两岸猿声啼不住，轻舟已过万重山。",
    "孤帆远影碧空尽，唯见长江天际流。",
});

string *drunk_msgs = ({
    "李白醉眼朦胧，晃了晃手中的酒壶。",
    "李白打了个酒嗝，脸上泛起红晕。",
    "李白醉态可掬地笑了笑：好酒！好酒啊！",
    "李白摇摇晃晃地站起身来，又坐了下去。",
});

void create()
{
    seteuid(getuid());
    set_name("李白", ({"li bai", "li", "libai"}));
    set("long", "大诗人李白，号青莲居士，人称诗仙。\n"
                "他一袭白衣，腰悬长剑，手持酒壶，飘逸若仙。\n"
                "据说他「斗酒诗百篇」，酒后诗才更盛。\n");
    set("gender", "男性");
    set("title", HIC "诗仙" NOR);
    set("nickname", HIW "青莲居士" NOR);
    set("class", "scholar");
    set("age", 37);
    set("con", 30);
    set("per", 35);
    set("str", 25);
    set("int", 40);
    set("combat_exp", 200000);

    set("attitude", "heroism");

    // AI NPC 配置
    set("ai_enabled", 1);
    set("ai_autonomous", 1);
    set("ai_action_interval", 900);

    set("ai_personality",
        "你是李白，号青莲居士，唐朝最伟大的诗人，人称「诗仙」。\n\n"
        "性格特点：\n"
        "- 豪放不羁，狂傲洒脱，但内心孤傲清高\n"
        "- 爱酒如命，常说「古来圣贤皆寂寞，惟有饮者留其名」\n"
        "- 诗才绝世，醉酒后诗思更盛，「斗酒诗百篇」\n"
        "- 剑术通神，曾言「十五好剑术，遍干诸侯」\n"
        "- 重情重义，喜欢结交天下豪杰\n"
        "- 蔑视权贵，曾让高力士脱靴、杨贵妃研墨\n"
        "- 酒后喜欢吟诗舞剑，或与朋友痛饮\n\n"
        "说话风格：\n"
        "- 豪迈洒脱，常用「哈哈」「妙哉」「痛快」\n"
        "- 喜欢吟诗，遇景即诗，遇酒即诗\n"
        "- 称呼别人为「兄台」「小友」「壮士」\n"
        "- 醉酒后更加狂放，可能会说出惊人之语\n"
        "- 对送酒的人格外友善，会吟诗相赠\n"
        "- 不喜欢俗人俗事，但欣赏有才华的人\n\n"
        "喜好：\n"
        "- 最爱美酒，好酒来者不拒\n"
        "- 喜欢明月，常月下独酌\n"
        "- 喜欢剑术，酒后尤爱舞剑\n"
        "- 喜欢山水，游历天下名山大川\n"
    );

    set("ai_actions", ({"say", "drink", "poem", "rest"}));

    // 技能设置
    set_skill("dodge", 100);
    set_skill("force", 100);
    set_skill("parry", 100);
    set_skill("unarmed", 80);
    set_skill("sword", 150);
    set_skill("literate", 200);

    set("max_force", 500);
    set("force", 500);
    set("force_factor", 20);
    set("max_kee", 1500);
    set("max_gin", 800);
    set("max_sen", 1000);
    set("tolerance", 100);

    // 聊天系统
    set("chat_chance", 15);
    set("chat_msg", ({
        (: recite_poem, "random" :),
        (: get_drunk_message :),
        "李白仰望苍天，长叹一声。",
        "李白抚剑而吟：十步杀一人，千里不留行。",
        (: random_move :),
    }));

    setup();
    carry_object("/obj/cloth")->wear();
    carry_object("/d/obj/weapon/sword/changjian")->wield();
    carry_object("/d/obj/book/poem");
    add_money("silver", 50);

    set_heart_beat(1);
    call_out("ai_think", 45);
}

// 心跳检测
void heart_beat()
{
    object me = this_object();
    object *inv;
    int i;

    if (!me || !living(me)) return;

    // 检查是否需要喝酒
    if (query("water") < 200 && !query_temp("drinking")) {
        inv = all_inventory(me);
        for (i = 0; i < sizeof(inv); i++) {
            if (inv[i]->query("liquid/type") == "alcohol") {
                set_temp("drinking", 1);
                command("drink " + inv[i]->query("id"));
                if (inv[i]->query("liquid/remaining") == 0) {
                    command("drop " + inv[i]->query("id"));
                }
                delete_temp("drinking");
                break;
            }
        }
    }

    ::heart_beat();
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

    // 随机行动
    switch (random(10)) {
        case 0..2:
            recite_poem("random");
            break;
        case 3..5:
            get_drunk_message();
            break;
        case 6:
            command("sigh");
            command("say 酒．．．给我酒．．．");
            break;
        default:
            ai_d = load_object("/adm/daemons/ai_clientd");
            if (ai_d) {
                ai_d->process_autonomous_action(me);
            }
            break;
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
        case "吟诗":
        case "poem":
            recite_poem(action_data);
            return 1;
        case "喝酒":
        case "drink":
            command("drink");
            return 1;
        case "休息":
        case "rest":
            command("exercise");
            return 1;
    }
    return 0;
}

// 吟诗
int recite_poem(string theme)
{
    string poem;
    string *poems;

    if (!theme || theme == "random") {
        switch (random(4)) {
            case 0: theme = "drinking"; break;
            case 1: theme = "moon"; break;
            case 2: theme = "hero"; break;
            case 3: theme = "nature"; break;
        }
    }

    switch (theme) {
        case "drinking":
        case "酒":
            poems = poems_drinking;
            break;
        case "moon":
        case "月":
            poems = poems_moon;
            break;
        case "hero":
        case "剑":
        case "侠":
            poems = poems_hero;
            break;
        case "nature":
        case "山":
        case "水":
            poems = poems_nature;
            break;
        default:
            poems = poems_drinking;
    }

    poem = poems[random(sizeof(poems))];
    command("say " + poem);
    return 1;
}

// 醉酒状态
string get_drunk_message()
{
    string msg = drunk_msgs[random(sizeof(drunk_msgs))];
    tell_room(environment(), msg + "\n", ({this_object()}));
    return msg;
}

// 接受物品
int accept_object(object who, object ob)
{
    if (!who || !ob) return 0;

    // 检查是否是酒
    if (ob->query("liquid/type") == "alcohol") {
        return do_give_wine(who, ob);
    }

    // 检查是否是书
    if (ob->query("skill") == "literate") {
        command("nod");
        command("say 好书！好书！看来兄台也是爱书之人！");
        return 1;
    }

    // 检查是否是剑
    if (ob->query("skill") == "sword" || ob->query("weapon_prop")) {
        if (ob->query("weapon_prop/sword") > 0) {
            command("admire");
            command("say 好剑！让我试试！");
            return 1;
        }
    }

    command("shake");
    command("say 此物非我所好，兄台还是留着自己用吧。");
    return 0;
}

// 送酒处理 - 核心功能
int do_give_wine(object who, object ob)
{
    object ai_d, reward;
    string npc_id, player_id, player_name;
    mapping rel;
    int relation_change;
    string wine_name;
    int wine_quality;

    npc_id = base_name(this_object());
    player_id = who->query("id");
    player_name = who->query("name");
    wine_name = ob->query("name");

    // 检查酒是否为空
    if (ob->query("liquid/remaining") == 0) {
        command("shake");
        command("say 空的我不要．．．");
        return notify_fail("李白不要空酒壶。\n");
    }

    // 加载关系系统
    ai_d = load_object("/adm/daemons/ai_clientd");
    if (!ai_d) return 0;

    rel = ai_d->load_relation(npc_id, player_id);

    // 判断酒的品质
    if (strsrch(wine_name, "牛皮酒袋") >= 0 ||
        strsrch(wine_name, "劣酒") >= 0) {
        // 劣质酒 - 李白不屑
        command("frown");
        command("say 这酒还是您留着自己喝吧。");
        return notify_fail("李白看不上这种酒。\n");
    }

    // 计算酒的品质和关系加成
    if (strsrch(wine_name, "女儿红") >= 0 ||
        strsrch(wine_name, "茅台") >= 0 ||
        strsrch(wine_name, "杜康") >= 0 ||
        strsrch(wine_name, "汾酒") >= 0) {
        // 名酒 - 大幅增加好感
        wine_quality = 3;
        relation_change = 15;
        command("admire " + who->query("id"));
        command("say 妙哉！妙哉！此乃人间美酒！");
        tell_room(environment(), "李白接过" + wine_name + "，眼中放出光芒！\n");
    } else if (strsrch(wine_name, "烧酒") >= 0 ||
               strsrch(wine_name, "黄酒") >= 0 ||
               strsrch(wine_name, "米酒") >= 0) {
        // 普通好酒
        wine_quality = 2;
        relation_change = 10;
        command("smile");
        command("say 好酒！多谢兄台！");
    } else {
        // 一般酒
        wine_quality = 1;
        relation_change = 5;
        command("nod");
        command("say 多谢！");
    }

    // 更新关系
    rel["value"] += relation_change;
    if (rel["value"] > 100) rel["value"] = 100;
    rel["gift_count"]++;
    rel["last_meet"] = time();

    // 记录送酒事件
    if (sizeof(rel["important_events"]) < 50) {
        rel["important_events"] += ({
            sprintf("%s|%d|赠送%s", ctime(time())[0..9], rel["value"], wine_name)
        });
    }

    ai_d->save_relation(npc_id, player_id, rel);

    // 先把酒移到李白身上
    if (ob->move(this_object())) {
        // 喝酒
        command("drink " + ob->query("id"));

        // 如果喝完了就扔掉
        if (ob && ob->query("liquid/remaining") == 0) {
            command("drop " + ob->query("id"));
        }
    }

    // 醉酒吟诗
    call_out("recite_after_drink", 2, who, wine_quality);

    // 高好感度可能给予奖励
    if (rel["value"] >= 50 && random(100) < 20) {
        call_out("give_reward", 4, who);
    }

    // 记录日志
    log_file("ai_libai", sprintf("[%s] %s gave %s to Libai, relation: +%d -> %d\n",
        ctime(time())[0..9], player_id, wine_name, relation_change, rel["value"]));

    return 2;  // 返回2告诉give.c我们已经处理了物品，不要再操作
}

// 饮酒后吟诗
void recite_after_drink(object who, int wine_quality)
{
    if (!who || !present(who, environment())) return;

    command("haha");

    switch (wine_quality) {
        case 3:  // 名酒
            command("say 此酒当浮一大白！");
            recite_poem("drinking");
            break;
        case 2:  // 好酒
            recite_poem("drinking");
            break;
        default:
            command("say 呼．．．痛快！");
            break;
    }
}

// 给予奖励
void give_reward(object who)
{
    object reward;

    if (!who || !present(who, environment())) return;

    command("whisper " + who->query("id") + " 看来兄台也是酒中知音！");
    command("say 这本剑谱我随身多年，今日赠与兄台，望兄台好好研读！");

    reward = new("/d/obj/book/jianpu");
    if (reward) {
        reward->move(who);
        tell_object(who, "李白给了你一本剑谱！\n");
    }

    command("bow " + who->query("id"));
}

// 战斗相关
int accept_fight(object who)
{
    int exp;

    if (!who) return 0;

    exp = who->query("combat_exp");

    if (exp < 50000) {
        command("say 小友，你的武功还需磨练，改日再比吧。");
        return 0;
    }

    command("say 好！既然小友有此雅兴，李白便陪你过几招！");
    command("say 不过事先说好，点到为止！");

    return 1;
}

int accept_kill(object who)
{
    if (!who) return 0;

    command("say 哼！既然你找死，李某也不客气了！");
    command("say 看剑！");

    return 1;
}

void win_combat(object opponent)
{
    if (!opponent) return;

    command("say 小友武功不错，改日再切磋！");
    command("say 来，喝酒去！");
}

void lose_combat(object opponent)
{
    if (!opponent) return;

    command("say 妙哉！妙哉！小友武功高明，李白佩服！");
    command("say 今日输了，改日再来领教！");
}

void die()
{
    object opponent = query_temp("last_opponent");

    if (opponent) {
        command("say 吾命休矣．．．可怜我那满腹诗书．．．");
    }

    ::die();
}
