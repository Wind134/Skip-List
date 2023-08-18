#include<iostream>
#include<cmath>
#include<cstring>
#include<mutex>
#include<fstream>
#define STORE_FILE "./store/dumpFile"

/**
 * @brief 这是一个链表中节点的数据结构；
 */
template<typename K,typename V>
class Node{
public:
    Node(){}
    Node(K k, V v, int);
    ~Node();
    K get_key() const;
    V get_value() const;
    void set_value(V);

    Node <K,V> **forward; // 该指针指向存储每层跳表节点节点的数组，数组大小取决于所在层，也就是说是一个指向数组(数组名本身也是指针)的指针
    int node_level;       // 节点所在层
private:
     K key;
     V value;
};

/**
 * @brief 跳表节点的构造函数，主要的工作是初始化跳表内容；
 * @param k 键；
 * @param v 值；
 * @param level 节点所处层级，从0开始；
 */
template<typename K,typename V>
Node<K,V>::Node(const K k, const V v, int level)
{
    this->key = k;
    this->value = v;
    this->node_level = level;
    this->forward = new Node<K,V> *[level + 1]; // 分配一个元素类型为指向Node节点指针的数组，forward指向该数组
    memset(this->forward, 0, sizeof(Node<K,V>*) * (level + 1));   // 将这段数组空间地址中的内容全赋为0
};

/**
 * @brief 析构函数，释放数组空间；
 */
template<typename  K,typename V>
Node<K,V>::~Node()
{
    delete [] forward;   // 清空分配的数组空间
};

/**
 * @brief 获取键key；
 * @return 返回key，成员函数为const版本；
 */
template<typename K,typename V>
K Node<K,V>::get_key() const {
    return key;
};

/**
 * @brief 获取值value；
 * @return 返回value值，成员函数为const版本；
 */
template<typename K,typename V>
V Node<K,V>::get_value() const {
    return value;
};

/**
 * @brief 设置值；
 * @param value 要设置的值；
 */
template<typename K,typename V>
void Node<K,V>::set_value(V value)
{
    this->value=value;
};

/*
-----------------------------------------------------------------------------
*/

/**
 * @brief 这是跳表的数据结构；
 */
template<typename K,typename V>
class SkipList{
public:
    SkipList(int);
    ~SkipList();
    int get_random_level();
    Node<K,V>*create_node(K,V,int);
    int insert_element(K,V);
    void display_list();
    bool search_element(K);
    void delete_element(K);
    void dump_file();
    void load_file(const std::string&);
    int size();
private:
    void get_key_value_from_string(const std::string &str,std::string*key,std::string *value);
    bool is_valid_string(const std::string &str);
private:
    int _max_level;              // 跳表的最大层级
    int _skip_list_level;        // 当前跳表的有效层级
    Node<K,V> *_header;          // 指向当前跳表的头节点的指针(指向最底层链表的最开始的那个结点)
    std::ofstream _file_writer;  // 默认以输入(writer)方式打开文件。
    std::ifstream _file_reader;  // 默认以输出(reader)方式打开文件。
    int _element_count;          // 表示跳表中元素的数量(只含实际存储的链表结点，不包括索引)
    static std::mutex mtx;
    static std::string delimiter;// 存放到STORE_FILE中时，将delimiter也存入进文件中，用于get_key_value_from_string的key与value区分
};

template<typename K,typename V>
std::mutex SkipList<K, V>::mtx; // 锁

template<typename K,typename V>
std::string SkipList<K, V>::delimiter = ":";    // 分隔点

/**
 * @brief 跳表的构造函数；
 * @param max_level 跳表的最大层数；
 */
template<typename K,typename V>
SkipList<K,V>::SkipList(int max_level)
{
    this->_max_level = max_level;
    this->_skip_list_level = 0; // 默认在第一层
    this->_element_count = 0;
    K k;    // 键
    V v;    // 值
    this->_header = new Node<K,V>(k, v, _max_level);    // 跳表的首节点，包含了最大层数的信息，初始都为空(取决于k v的默认初始化)
};

/**
 * @brief 跳表的析构函数，关闭正在运行的输入输出字节流，同时释放存储跳表首结点地址的数组空间
 */
template<typename K, typename V>
SkipList<K,V>::~SkipList()
{
    if(_file_writer.is_open())
    {
        _file_writer.close();
    }
    if(_file_reader.is_open())
    {
        _file_reader.close();
    }
    delete _header;
}

/**
 * @brief 根据指定的参数创建一个新节点；
 * @param k 键；
 * @param v 值；
 * @param level 所在层级；
 * @return 指向这个新创建节点的指针；
 */
template<typename K,typename V>
Node<K,V> *SkipList<K,V>::create_node(const K k, const V v, int level)
{
    Node<K,V> *n = new Node<K, V>(k, v, level); // 所以说，每个节点所带的level层级可能是不一样的；
    return n;
}


/**
 * @brief 根据指定的参数插入一个新节点，通过遍历跳表，找到插入位置，并根据随机层级创建节点，若键已存在则表明插入失败；
 * @param key 键；
 * @param value 值；
 * @return 插入失败返回1，插入成功返回0；
 */
template<typename K,typename V>
int SkipList<K,V>::insert_element(const K key,const V value)
{
    mtx.lock(); // 互斥访问，实现插入过程的线程安全；
    Node<K,V> *current = this->_header; // 当前层级链表的头结点(头结点永远不会变，但current结点会不断移动)
    Node<K,V> *update[_max_level];      // 存储每一层都需要更新的节点信息，后面要用，预先分配最大的空间量
    memset(update, 0, sizeof(Node<K,V>*) * (_max_level+1)); // 先将这段数组空间清零(初始化)

    for(int i = _skip_list_level; i >= 0; --i)  // 从当前列表层级开始，逐步向下，从这里可以看到的是，每层的索引都会被更新    
    {
        // 只要该层链表头结点不为空，同时该层头结点的key要小于要插入的key
        while(current->forward[i] && current->forward[i]->get_key() < key) // 从这里可以看到，key要求有序
        {
            current = current->forward[i];  // current更新为该节点在第i层的下一个节点
        }
        update[i] = current;   // update是存储每一层(包括索引层)需要插入节点的位置，因为每层都需要更新(每层的结点都是一致的)；
    }

    current = current->forward[0];  // 指向下一个结点的最底层的结点，因为要准备插入结点信息了

    if(current && current->get_key() == key)
    {
        std::cout<<"key: " << key << " exists in skiplist!" << std::endl;
        mtx.unlock();   // 归还锁，避免死锁；
        return 1;
    }

    // 添加的值没有在跳表中，或者整个跳表为空
    if(!current || current->get_key() != key)
    {
        int random_level = get_random_level();  // 获得一个随机层
        if(random_level > _skip_list_level)     // 如果给定的随即层大于当前所在层，那需要将多出的层初始化
        {
            for(int i = _skip_list_level + 1; i < random_level + 1; ++i)
            {
                update[i] = _header;            // 初始化每一层的update节点为header节点
            }
            _skip_list_level = random_level;    // 更新所在层
        }

        Node<K,V>* inserted_node = create_node(key, value, random_level);   // 根据给定的层信息建立一个节点

        for(int i = 0; i < random_level; i++)   // 从最底层(单链表)那一层开始，每一层相应位置都插入节点信息
        {
            inserted_node->forward[i] = update[i]->forward[i];  // 跟链表的插入元素操作一样(forward[i]就类似next指针，好好理解这一点)
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key: " << key << ", value: " << value << std::endl;
        ++_element_count;   // 跳表新增一个节点数，从这里可以看出，索引节点是不算数的
    }
    mtx.unlock();   // 归还锁
    return 0;
}


/**
 * @brief 输出跳表(0 ~ 当前层)中包含的所有内容；
 */
template<typename K, typename V>
void SkipList<K, V>::display_list()
{
    std::cout<<"\n********************SkipList********************"<<"\n";
    for(int i = 0; i < _skip_list_level; ++i)
    {
        Node<K, V> *node = this->_header->forward[i];   // 每一层的开始节点
        std::cout << "Level " << i << ": ";
        while(node)
        {
            std::cout<< "{" << node->get_key() <<": "<<node->get_value()<<"}; ";
            node = node->forward[i];    // 该层当前结点的下一个结点
        }
        std::cout << std::endl;
    }
}

/**
 * @brief 将跳跃表的内容持久化到文件中，遍历跳跃表的每个节点，将键值对写入文件。
 */
template<typename K, typename V>
void SkipList<K,V>::dump_file()
{
    std::cout<<"dump_file--------------------------"<<std::endl;
    _file_writer.open(STORE_FILE);  // 以写入流的方式打开文件
    Node<K,V> *node = this->_header->forward[0];    // 最底层的首结点
    while(node)
    {
        _file_writer << node->get_key() << ": " << node->get_value() << "\n";
        std::cout << node->get_key() << ": " << node->get_value() << "\n";
        node = node->forward[0];
    }
    _file_writer.flush();   // 设置写入文件缓冲区函数
    _file_writer.close();   // 关闭文件
}

/**
 * @brief 从文件中提取字符串流，将符合键值(key, value)格式的数据全部存入跳表；
 */
template<typename K, typename V>
void SkipList<K, V>::load_file(const std::string& str)
{
    _file_reader.open(str);
    std::cout<<"load_file---------------------------"<<std::endl;
    std::string line;
    std::string *key = new std::string();
    std::string *value = new std::string();
    while(getline(_file_reader, line))
    {
        // std::cout << line << std::endl;
        // std::cout << std::boolalpha << is_valid_string(line) << std::endl;
        get_key_value_from_string(line, key, value);    // 从每一行字符串流中提取相应信息
        // std::cout << *key << " " << *value << std::endl;
        if(key->empty() || value->empty())  // 有一个为空则继续
        {
            continue;
        }
        int target = 0;
        std::string str_key= *key;   // 当时定义的key为int类型，所以将得到的string类型的key转成int
        // 下面是一个基础的转换算法，string类型的数字转换成整型的数字
        for(int i = 0;i < str_key.size();i++)
        {
            target = target * 10 + str_key[i] - '0';
        }
        int Yes_No = insert_element(target, *value);    // 插入的数据
        std::cout << "key: "<< *key << ", value: " << *value << std::endl;   // 输出文件中转换后的结果
    }
    _file_reader.close();
}

/**
 * @brief 获取跳表中元素的数量；
 * @return 元素数量值；
 */
template<typename K,typename V>
int SkipList<K, V>::size() {
    return _element_count;
}

/**
 * @brief 从字符串中获取key和value值；
 * @param str 要读取的字符串；
 * @param key 指向key值的地址的指针；
 * @param value 指向value值的地址的指针；
 */
template<typename K,typename V>
void SkipList<K,V>::get_key_value_from_string(const std::string &str, std::string *key, std::string *value)
{
    if(!is_valid_string(str)) return ;
    *key = str.substr(0, str.find(delimiter));  // 提取key
    auto it = str.find(delimiter) + 1;
    while (it != std::string::npos && str[it] == ' ') ++it;   // 加了一个循环处理空格 
    *value = str.substr(it, str.length()); // 提取string
}

/**
 * @brief 判断字符串的有效性；
 * @param str 要判断的字符串
 * @return 判断结果；
 */
template<typename K,typename V>
bool SkipList<K, V>::is_valid_string(const std::string &str)
{
    if(str.empty()) // 字符为空，错；
    {
        return false;
    }
    if(str.find(delimiter) == std::string::npos)  // 没有';'号，错；
    {
        std::cout << delimiter << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief 删除有关节点(每一层)
 * @param key 要删除的结点键值
 */
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key)
{
    mtx.lock(); // 线程安全
    Node<K, V> *current = this->_header;
    Node<K, V> *update[_max_level + 1];     // 预定义指针
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level+1));    // 初始化那一片空间
    for(int i = _skip_list_level; i >= 0; --i)
    {
        while(current->forward[i] && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];  // 不断循环，定位到要删除的结点
        }
        // 每一层要删除的结点保存在数组中
        // 如果找到了目标结点，这里保存的是目标结点的前一个结点
        // 如果没找到目标结点，该节点要么保存的是最后一个结点，要么是小于那个结点值的最大值
        update[i] = current;
    }

    current = current->forward[0];  // 指向底层的相应结点

    if(current && current->get_key() == key)
    {
        for (int i = 0; i <= _skip_list_level; ++i) {
            // 由于某些键值并不一定存在于某层，因此这种情况
            if (update[i]->forward[i] != current) {
                std::cout << "The key isn't exist in level " << i << std::endl;
                // 使用break是因为，如果这一层都不存在该索引，显然往上更不可能存在
                // 因为每一层的值都来源于上一层，如果下一层某个值不存在，那么往上更不可能存在；
                break;
            }
            // 中间的结点值不需要delete吗，不需要，因为Node结点已经自行设定了析构函数
            update[i]->forward[i] = current->forward[i];
        }
        while(_skip_list_level > 0 && _header->forward[_skip_list_level] == nullptr)    // 删除全为0的层(从上到下依次删)
        {
            --_skip_list_level;
        }
        std::cout<<"Successfully deleted key " << key <<std::endl;
        --_element_count;
    }
    mtx.unlock();
}

/**
 * @brief 搜索指定键的信息；
 * @param key 要搜索的键；
 * @return 搜索的结果；
 */
template<typename K,typename V>
bool SkipList<K,V>::search_element(K key)
{
    std::cout<<"search_element--------------------------"<<std::endl;
    Node<K, V> *current = _header;
    for(int i = _skip_list_level; i >= 0 ; --i) // 从最高层开始
    {
        while(current->forward[i] && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];
        }
    }
    current = current->forward[0];  // 更新current到最底层链表的结点，这个结点如果找到了对应值那就是结点了
    if(current && current->get_key() == key )
    {
        std::cout << "Found key: " << key <<", value: " << current->get_value() << std::endl;
        return true;
    }
    std::cout<<"Not Found Key: "<< key << std::endl;
    return false;
}

/**
 * @brief 生成一个随机层级，从第一层开始，每一层以50%的概率加入；
 * @return 返回一个随机的层级；
 */
template<typename K,typename V>
int SkipList<K,V>::get_random_level()
{
    int k = 1;
    while(rand() % 2)   // 生成一个随机整数，奇数则增
    {
        ++k;
    }
    k = (k<_max_level) ? k : _max_level;    // 但不能无限增长，需要用max_level限制
    return k;
};