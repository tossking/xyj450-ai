// relation.c - 查询与NPC的关系
// 玩家可以使用 relation <NPC> 查看关系

#include <ansi.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
    object npc;
    string npc_id, player_id, player_name;
    mapping rel;
    string output;
    int value;
    string title;

    if (!arg) {
        return notify_fail("指令格式: relation <NPC名称>\n"
                          "例如: relation rulai\n"
                          "      relation 如来佛\n");
    }

    // 尝试查找NPC
    npc = present(arg, environment(me));
    if (!npc) {
        // 尝试按中文名查找
        object *inv = all_inventory(environment(me));
        for (int i = 0; i < sizeof(inv); i++) {
            if (inv[i]->is_character() && !userp(inv[i])) {
                if (strsrch(inv[i]->query("name"), arg) >= 0 ||
                    strsrch(inv[i]->query("id"), arg) >= 0) {
                    npc = inv[i];
                    break;
                }
            }
        }
    }

    if (!npc) {
        return notify_fail("你周围没有这个人。\n");
    }

    if (!npc->query("ai_enabled")) {
        return notify_fail(npc->query("name") + "不是AI NPC，没有关系系统。\n");
    }

    player_id = me->query("id");
    player_name = me->query("name");
    npc_id = base_name(npc);

    // 加载关系数据
    object ai_d = load_object("/adm/daemons/ai_clientd");
    if (!ai_d) {
        return notify_fail("系统错误：AI守护进程未加载。\n");
    }

    rel = ai_d->query_relation_detail(npc_id, player_id);
    value = rel["value"];

    // 根据关系值确定称号
    if (value >= 80) title = HIC "挚友" NOR;
    else if (value >= 50) title = HIG "友好" NOR;
    else if (value >= 20) title = HIY "相识" NOR;
    else if (value >= -19) title = WHT "陌生人" NOR;
    else if (value >= -49) title = CYN "疏远" NOR;
    else if (value >= -79) title = RED "敌对" NOR;
    else title = HIR "仇敌" NOR;

    output = sprintf("\n" HIW "═══════════════════════════════════════\n" NOR);
    output += sprintf(HIW "【关系查询】%s 与 %s\n" NOR, player_name, npc->query("name"));
    output += sprintf(HIW "═══════════════════════════════════════\n" NOR);
    output += sprintf("关系等级：%s (%d)\n", title, value);
    output += sprintf("见面次数：%d 次\n", rel["meet_count"]);
    output += sprintf("对话次数：%d 次\n", rel["talk_count"]);

    if (rel["quest_count"] > 0) {
        output += sprintf("完成任务：%d 次\n", rel["quest_count"]);
    }
    if (rel["gift_count"] > 0) {
        output += sprintf("赠送礼物：%d 次\n", rel["gift_count"]);
    }
    if (rel["first_meet"] > 0) {
        output += sprintf("初次见面：%s\n", ctime(rel["first_meet"])[0..9]);
    }
    if (rel["last_meet"] > 0) {
        output += sprintf("最后见面：%s\n", ctime(rel["last_meet"])[0..9]);
    }

    // 显示最近的重要事件
    mixed *events = rel["important_events"];
    if (events && sizeof(events) > 0) {
        output += sprintf("\n" HIY "【最近事件】\n" NOR);
        int start = sizeof(events) - 5;
        if (start < 0) start = 0;
        for (int i = start; i < sizeof(events); i++) {
            output += sprintf("  %s\n", events[i]);
        }
    }

    output += sprintf(HIW "═══════════════════════════════════════\n" NOR);

    write(output);
    return 1;
}

int help(object me)
{
    write(@HELP
指令格式: relation <NPC名称>

查询你与AI NPC的关系。

关系等级说明：
  挚友(80~100)：最亲密的朋友
  友好(50~79)：值得信赖的朋友
  相识(20~49)：有点印象的熟人
  陌生人(-19~19)：初次见面或泛泛之交
  疏远(-49~-20)：有些不愉快
  敌对(-79~-50)：明显的敌意
  仇敌(-100~-80)：不共戴天

提升关系的方法：
  - 与NPC对话（每10次+1关系）
  - 完成NPC发布的任务（+10关系）
  - 赠送NPC喜欢的礼物

注意：只有AI NPC才有关系系统。

示例：
  relation rulai    - 查询与如来佛的关系
  relation 如来佛    - 同上
HELP);
    return 1;
}
