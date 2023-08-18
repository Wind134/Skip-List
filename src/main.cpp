//所有函数的解释与用法都在skiplist.h中，main主函数主要用于测试各种函数是否可行

#include "skiplist.h"
using namespace std;

void error_handling(const string& message)
{
    cerr << message << endl;
    exit(1);
}

int main(int argc, char* argv[])
{
    SkipList<int, std::string> skipList(6);
    if (argc < 2) error_handling("You should input the path of data file!\nPlease try again!");
    skipList.load_file(argv[1]);

    cout << "skipList.size = " << skipList.size() << endl;
    
    skipList.search_element(1);
    skipList.search_element(5);
    skipList.display_list();

    skipList.dump_file();
    skipList.delete_element(3);
    skipList.delete_element(9);

    cout << "skipList.size = " << skipList.size() << endl;
    return 0;
}