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
    int64_t input;
    std::string value;
    char value_result[120];
    int table_id;

    if (argc > 1) {
        f_in.open(argv[1]);
        use_file = true;

        std::cin.rdbuf(f_in.rdbuf());
        std::cout << f_in.fail();

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
            std::cin >> input;
            std::cin.ignore(INT_MAX, '\n');
            if (db_delete(input) == 0) {
                std::cout << "delete success: " << input << '\n';
            } else {
                std::cout << "delete failed: " << input << '\n';
            }
            break;
        case 'i':
            std::cin >> input;
            std::cin.get();
            std::getline(std::cin, value);
            std::cout << "insert: " << input << ", " << value << '\n';
            if (db_insert(input, &value[0]) == 0) {
                std::cout << "insert success: " << input << ", " << value << '\n';
            } else {
                std::cout << "insert failed: " << input << ", " << value << '\n';
            }
            //root = insert_key(root, input, input);
            //print_tree(root);
            break;
        case 'f':
            std::cin >> input;
            std::cin.ignore(INT_MAX, '\n');
            if (db_find(input, value_result) == 0) {
                std::cout << "find success: " << input << ", " << value_result << '\n';
            } else {
                std::cout << "find failed: " << input << ", " << value_result << '\n';
            }
            //root = insert_key(root, input, input);
            //print_tree(root);
            break;
        case 'p':
            std::cin.ignore(INT_MAX, '\n');
            db_print_tree(std::cout);
            break;
        case 'l':
            std::cin.ignore(INT_MAX, '\n');
            db_print_leaf(std::cout);
            break;
        case 'o':
            std::cin.get();
            std::getline(std::cin, value);
            table_id = open_table(&value[0]);
            std::cout << "open: table_id - : " << table_id << '\n';
            //print_leaves(root);
            break;
        case 's':
            std::cin >> input;
            std::cin.ignore(INT_MAX, '\n');
            if (set_db_file(input) >= 0) {
                std::cout << "set success: " << input << '\n';
            } else {
                std::cout << "set failed: " << input << '\n';
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
