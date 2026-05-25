// REWARD_D - 奖励守护进程
// 处理解谜任务和特殊奖励

#include <ansi.h>

void create()
{
    seteuid(getuid());
}

// 检查玩家是否有某个解谜任务
int riddle_check(object who, string riddle)
{
    if (!who || !riddle) return 0;
    return who->query("riddle/" + riddle) > 0;
}

// 检查玩家是否已完成某个解谜任务
int check_m_success(object who, string riddle)
{
    if (!who || !riddle) return 0;
    return who->query("m_success/" + riddle) > 0;
}

// 标记解谜任务完成
int riddle_done(object who, string riddle, int exp_bonus, int silent)
{
    if (!who || !riddle) return 0;

    who->set("m_success/" + riddle, 1);
    who->add("combat_exp", exp_bonus);

    if (!silent) {
        tell_object(who, HIY "你完成了『" + riddle + "』任务！\n" NOR);
        tell_object(who, YEL "你获得了" + chinese_number(exp_bonus) + "点实战经验！\n" NOR);
    }

    return 1;
}

// 设置解谜任务
int riddle_set(object who, string riddle, int step)
{
    if (!who || !riddle) return 0;
    who->set("riddle/" + riddle, step);
    return 1;
}
