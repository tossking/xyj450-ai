// QUESTS_D - 任务系统守护进程
// 处理风云任务的发放、验证和奖励

#include <ansi.h>

void create()
{
    seteuid(getuid());
}

// 任务等级对应的银子奖励
int get_silver_bonus(int exp_bonus) {
    return exp_bonus * 2;  // 银子奖励为经验的2倍
}

// 根据等级获取取消任务费用
int get_cancel_fee(object who) {
    int level;
    int exp = who->query("combat_exp");

    if (exp >= 4000000) return 200;      // 4M+
    if (exp >= 2000000) return 150;      // 2M+
    if (exp >= 1000000) return 100;      // 1M+
    if (exp >= 100000) return 50;        // 100K+
    return 25;                            // 新手
}

// 发放任务
int give_quest(object who, mapping info) {
    mapping quest;
    string tag;
    int j, combatexp, timep;
    string *levels = ({
        "0", "4000", "8000", "16000", "32000", "64000",
        "128000", "256000", "512000", "1024000", "1536000",
        "2304000", "3456000", "5184000", "7776000", "11664000",
        "17496000", "26244000", "40000000"
    });

    if (!who || !living(who)) return 0;

    // 检查是否是鬼魂
    if (who->is_ghost()) {
        tell_object(who, "鬼魂不可要任务。\n");
        return 0;
    }

    // 检查是否已有任务
    if (quest = who->query("fy_quest")) {
        if (who->query("task_time") > time()) {
            tell_object(who, "你还有未完成的任务。\n");
            return 0;
        }
    }

    // 根据经验选择任务等级
    combatexp = who->query("combat_exp");
    tag = "40000000";
    for (j = sizeof(levels) - 1; j >= 0; j--) {
        if (atoi(levels[j]) <= combatexp) {
            tag = levels[j];
            break;
        }
    }

    // 随机选择特殊任务
    if (!random(40)) tag = "_new";
    if (!random(10)) tag = "_common";

    // 获取任务
    quest = QUEST_D(tag)->query_quest();

    // 设置时间（根据经验调整）
    timep = 600;  // 基础10分钟
    if (combatexp > 1000000) timep = 480;  // 高手8分钟
    if (combatexp > 5000000) timep = 360;  // 顶级高手6分钟
    timep = random(timep/2) + timep/2;

    // 记录任务
    who->set("fy_quest", quest);
    who->set("task_time", time() + timep);
    who->set("quest_employer", info["employer"] || "天机老人");

    // 显示任务信息
    tell_object(who, HIW "\n═══════════════════════════════════════\n" NOR);
    if (quest["quest_type"] == "杀") {
        tell_object(who, HIW "【任务】替天机老人杀了『" + quest["quest"] + "』\n" NOR);
    } else {
        tell_object(who, HIW "【任务】找回『" + quest["quest"] + "』给天机老人\n" NOR);
    }

    // 计算时间显示
    {
        int t = timep;
        int s = t % 60; t /= 60;
        int m = t % 60; t /= 60;
        int h = t % 24;
        string time_str = "";
        if (h) time_str += chinese_number(h) + "小时";
        if (m) time_str += chinese_number(m) + "分";
        time_str += chinese_number(s) + "秒";
        tell_object(who, HIW "【时限】" + time_str + "\n" NOR);
    }

    // 显示奖励预览
    tell_object(who, HIW "【奖励】约" + quest["exp_bonus"] + "实战经验，" +
                get_silver_bonus(quest["exp_bonus"]) + "两银子\n" NOR);
    tell_object(who, HIW "═══════════════════════════════════════\n" NOR);

    // 广播任务
    CHANNEL_D->do_channel(this_object(), "qst",
        sprintf("%s(%s)：%s『%s』",
        who->query("name"), who->query("id"),
        quest["quest_type"] == "杀" ? "杀" : "寻",
        quest["quest"]));

    return 1;
}

// 验证任务完成
int verify_quest(object who, string quest_name) {
    mapping quest;

    if (!who) return 0;

    quest = who->query("fy_quest");
    if (!quest) return 0;

    if (quest["quest"] == quest_name ||
        strsrch(quest["quest"], quest_name) >= 0 ||
        strsrch(quest_name, quest["quest"]) >= 0) {
        return 1;
    }

    return 0;
}

// 给予奖励
int give_reward(object who, object employer, mapping quest) {
    int exp_bonus, silver_bonus;
    int time_used, time_limit;
    object money;
    string msg;

    if (!who || !quest) return 0;

    exp_bonus = quest["exp_bonus"];
    silver_bonus = get_silver_bonus(exp_bonus);

    // 计算时间奖励
    time_limit = 600;  // 默认时限
    if (who->query("task_time")) {
        time_used = who->query("task_time") - time();

        if (time_used > time_limit / 2) {
            // 提前一半时间完成，奖励翻倍
            exp_bonus *= 2;
            silver_bonus *= 2;
            msg = "干得漂亮！提前完成，奖励翻倍！";
        } else if (time_used > time_limit / 4) {
            // 提前1/4时间完成，奖励加半
            exp_bonus = exp_bonus * 3 / 2;
            silver_bonus = silver_bonus * 3 / 2;
            msg = "不错不错，效率挺高！";
        } else {
            msg = "还行，按时完成。";
        }
    }

    // 发放经验
    who->add("combat_exp", exp_bonus);
    tell_object(who, YEL "你获得了" + chinese_number(exp_bonus) + "点实战经验！\n" NOR);

    // 发放银子
    if (silver_bonus > 0) {
        money = new("/obj/money/silver");
        money->set_amount(silver_bonus);
        if (money->move(who)) {
            tell_object(who, YEL "你获得了" + chinese_number(silver_bonus) + "两银子！\n" NOR);
            if (employer) {
                tell_object(who, employer->query("name") + "给了你" +
                           chinese_number(silver_bonus) + "两银子！\n");
            }
        } else {
            destruct(money);
            tell_object(who, "你身上太重了，拿不动银子。\n");
        }
    }

    // 记录统计
    who->add("fy_quest_stats/completed", 1);
    who->add("fy_quest_stats/total_exp", exp_bonus);
    who->add("fy_quest_stats/total_silver", silver_bonus);

    // 清除任务数据
    who->delete("fy_quest");
    who->delete("task_time");
    who->delete("quest_employer");

    if (employer && msg) {
        tell_object(who, CYN + employer->query("name") + "说道：" + msg + "\n" NOR);
    }

    return 1;
}

// 特殊任务奖励
int special_reward(object who, string quest_name) {
    mapping quest;

    if (!who) return 0;

    quest = who->query("fy_quest");
    if (!quest) return 0;

    return give_reward(who, this_object(), quest);
}

// 接受任务物品
int accept_quest_object(object who, object ob, object employer) {
    mapping quest;
    string ob_name;

    if (!who || !ob) return 0;

    quest = who->query("fy_quest");
    if (!quest || quest["quest_type"] != "寻") return 0;

    ob_name = ob->query("name");

    // 检查是否是任务物品
    if (strsrch(ob_name, quest["quest"]) >= 0 ||
        strsrch(quest["quest"], ob_name) >= 0) {

        tell_object(who, CYN + employer->query("name") + "说道：好的，找到了！\n" NOR);
        destruct(ob);
        give_reward(who, employer, quest);
        return 1;
    }

    return 0;
}

// 等待时间显示
string wait_period(int timep) {
    int t = timep;
    int s = t % 60; t /= 60;
    int m = t % 60; t /= 60;
    int h = t % 24; t /= 24;
    int d = t;
    string time = "";

    if (d) time = chinese_number(d) + "天";
    if (h) time += chinese_number(h) + "小时";
    if (m) time += chinese_number(m) + "分";
    time += chinese_number(s) + "秒";

    return time;
}

// 迷宫奖励
int dungeon_reward(object leader, string quest_name, object member, int bonus) {
    int exp_bonus, silver_bonus;
    object money;

    if (!member) return 0;

    exp_bonus = 50 + random(50) + bonus;
    silver_bonus = exp_bonus * 2;

    member->add("combat_exp", exp_bonus);
    tell_object(member, YEL "你获得了" + chinese_number(exp_bonus) + "点实战经验！\n" NOR);

    if (silver_bonus > 0) {
        money = new("/obj/money/silver");
        money->set_amount(silver_bonus);
        if (money->move(member)) {
            tell_object(member, YEL "你获得了" + chinese_number(silver_bonus) + "两银子！\n" NOR);
        } else {
            destruct(money);
        }
    }

    return 1;
}
