// 神话世界·西游记·版本４．５０
/* <SecCrypt CPL V3R05> */

// cuff.c - 拳法基础技能

inherit SKILL;

mapping *action = ({
([      "action":               "$N挥拳向$n的$l打去",
        "dodge":                10,
        "parry":                5,
        "force":                30,
        "damage_type":  "瘀伤",
]),
([      "action":               "$N双拳齐出，直取$n的$l",
        "dodge":                5,
        "parry":                10,
        "force":                40,
        "damage_type":  "瘀伤",
]),
([      "action":               "$N一记勾拳击向$n的面门",
        "dodge":                15,
        "parry":                5,
        "force":                35,
        "damage_type":  "瘀伤",
]),
([      "action":               "$N侧身闪过，反手一拳打向$n的胸口",
        "dodge":                20,
        "parry":                10,
        "force":                45,
        "damage_type":  "瘀伤",
]),
([      "action":               "$N猛然出拳，直击$n的$l",
        "dodge":                10,
        "parry":                15,
        "force":                50,
        "damage_type":  "瘀伤",
]),
});

int valid_learn(object me)
{
        if( me->query_temp("weapon") || me->query_temp("secondary_weapon") )
                return notify_fail("练拳法必须空手。\n");
        return 1;
}

int valid_enable(string usage)
{
        return usage=="unarmed" || usage=="parry";
}

mapping query_action(object me, object weapon)
{
        return action[random(sizeof(action))];
}

int practice_skill(object me)
{
        if( (int)me->query("sen") < 30)
                return notify_fail("你的精神无法集中了，休息一下再练吧。\n");
        if( (int)me->query("kee") < 30 )
                return notify_fail("你现在手足酸软，休息一下再练吧。\n");
        if( (int)me->query("force") < 10 )
                return notify_fail("你的内力不够了。\n");

        me->receive_damage("sen", 30);
        me->receive_damage("kee", 30);
        me->add("force", -10);

        return 1;
}
