#include "person.pb.h"

int main()
{
    person::Contact contact;
    auto *stu = contact.add_students();
    // person::Student stu;
    stu->set_sn(10001);
    stu->set_name("张三");
    stu->add_hobby("打篮球");
    stu->add_hobby("踢足球");
    stu->add_hobby("打乒乓球");
    auto score = stu->mutable_score();
    score->insert({"语文", 77});
    score->insert({"数学", 88.5});
    score->insert({"英语", 85});
    stu->set_type(person::StuType::CADRE);

    stu->set_height(170);
    stu->set_weight(65);

    stu->set_birth("2008-08-08");



    std::string str = contact.SerializeAsString();
    std::cout << str << std::endl;

    // person::Student tmp;
    person::Contact test;
    test.ParseFromString(str);
    auto tmp = test.students(0); //获取第0个元素

    std::cout << tmp.name() << std::endl;
    std::cout << tmp.sn() << std::endl;
    for (int i = 0; i < tmp.hobby_size(); i++) {
        std::cout << tmp.hobby(i) << std::endl;
    }
    auto &cscore = tmp.score();
    auto *pscore = tmp.mutable_score();
    // float ch = (*pscore)["语文"];
    for (auto s: cscore) {
        std::cout << s.first << "=" << s.second << std::endl;
    }

    person::StuType type = tmp.type();
    std::cout << type << std::endl;

    if (tmp.has_height()) {
        std::cout << tmp.height() << std::endl;
    }
    if (tmp.has_weight()) {
        std::cout << tmp.weight() << std::endl;
    }

    if (tmp.has_birth()) {
        std::cout << "生日:" << tmp.birth() << std::endl;
    }

    return 0;
}