#include "bptapi.h"

#include <fstream>
#include <iostream>

// MAIN

int main( int argc, char ** argv ) {
    std::ifstream f_in;
    std::ofstream f_out;
    std::streambuf* old_cin_buf = std::cin.rdbuf();
    std::streambuf* old_cout_buf = std::cout.rdbuf();
    bool use_file = false;
    bool use_outfile = false;

    char instruction;
    int64_t table_id;
    int64_t input;
    std::string value;
    char value_result[120];
    int table_id_n;

    if (argc > 1) {
        f_in.open(argv[1]);
        use_file = true;

        std::cin.rdbuf(f_in.rdbuf());

        if (argc > 2) {
            f_out.open(argv[2]);
            use_outfile = true;
            std::cout.rdbuf(f_out.rdbuf());
        }
    }

    if (!use_file) {
        printf("> ");
    }

    while (!std::cin.eof()) {
        std::cin >> instruction;
        switch (instruction) {
        case 'd':
            std::cin >> table_id >> input;
            std::cin.ignore(INT_MAX, '\n');
            if (db_delete(table_id, input) == 0) {
                std::cout << "delete success: " << input << '\n';
            } else {
                std::cout << "delete failed: " << input << '\n';
            }
            break;
        case 'i':
            std::cin >> table_id >> input;
            std::cin.get();
            std::getline(std::cin, value);
            std::cout << "insert: " << table_id << " - " << input << ", " << value << '\n';
            if (db_insert(table_id, input, &value[0]) == 0) {
                std::cout << "insert success: " << table_id << "-" << input << ", " << value << '\n';
            } else {
                std::cout << "insert failed: " << table_id << "-" << input << ", " << value << '\n';
            }
            //root = insert_key(root, input, input);
            //print_tree(root);
            break;
        case 'f':
            std::cin >> table_id >> input;
            std::cin.ignore(INT_MAX, '\n');
            if (db_find(table_id, input, value_result) == 0) {
                std::cout << "find success: " << table_id << "-" << input << ", " << value_result << '\n';
            } else {
                std::cout << "find failed: " << table_id << "-" << input << ", " << value_result << '\n';
            }
            //root = insert_key(root, input, input);
            //print_tree(root);
            break;
        case 'p':
            std::cin >> table_id;
            std::cin.ignore(INT_MAX, '\n');
            db_print_tree(table_id, std::cout);
            break;
        case 'l':
            std::cin >> table_id;
            std::cin.ignore(INT_MAX, '\n');
            db_print_leaf(table_id, std::cout);
            break;
        case 'o':
            std::cin.get();
            std::getline(std::cin, value);
            table_id_n = open_table(&value[0]);
            if (table_id_n > 0) {
                std::cout << "open success: " << table_id_n << "-" << value << '\n';
            } else {
                std::cout << "open failed: " << table_id_n << "-" << value << '\n';
            }
            //print_leaves(root);
            break;
        case 'c':
            std::cin >> table_id;
            std::cin.ignore(INT_MAX, '\n');
            if (close_table(table_id) == 0) {
                std::cout << "close success: " << table_id << '\n';
            } else {
                std::cout << "close failed: " << table_id << '\n';
            }
            break;
        case 'z':
            std::cin >> input;
            std::cin.ignore(INT_MAX, '\n');
            if (init_db(input) == 0) {
                std::cout << "init success: " << input << '\n';
            } else {
                std::cout << "init failed: " << input << '\n';
            }
            break;
        case 's':
            std::cin.ignore(INT_MAX, '\n');
            if (shutdown_db() == 0) {
                std::cout << "shutdown success" << '\n';
            } else {
                std::cout << "shutdown failed" << '\n';
            }
            break;
        default:
            break;
        }
        if (!use_file) {
            printf("> ");
        }
    }
    std::cout << std::endl;

    if (use_file) {
        std::cin.rdbuf(old_cin_buf);
        f_in.close();
    }

    if (use_outfile) {
        std::cout.rdbuf(old_cout_buf);
        f_out.close();
    }

    return EXIT_SUCCESS;
}
