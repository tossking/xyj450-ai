// AI测试NPC
inherit NPC;

void create()
{
    set_name("智能老者", ({"aigo", "oldman", "old"}) );
    set("long", "一位看起来很有智慧的老人，似乎能回答任何问题。\n");

    set("gender", "男性");
    set("age", 80);
    set("attitude", "peaceful");

    set("str", 20);
    set("int", 30);
    set("con", 20);
    set("dex", 20);

    set("max_kee", 1000);
    set("kee", 1000);
    set("max_sen", 1000);
    set("sen", 1000);
    set("combat_exp", 100000);

    // AI NPC 配置
    set("ai_enabled", 1);
    set("ai_personality",
        "你是一位智慧的老者，名叫智能老者。\n"
        "你博学多才，见多识广，对武功、人生、世界都有深刻的见解。\n"
        "你说话温和睿智，喜欢用比喻和故事来解释道理。\n"
        "你对年轻人很友善，愿意分享你的智慧。"
    );

    set_skill("unarmed", 50);
    set_skill("dodge", 50);
    set_skill("parry", 50);

    setup();
}
