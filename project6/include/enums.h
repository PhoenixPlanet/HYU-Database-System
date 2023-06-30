#ifndef __ENUMS_H__
#define __ENUMS_H__

enum class NodeType {
    HEADER,
    INTERNAL,
    LEAF
};

enum class LogType {
    BEGIN = 0,
    UPDATE = 1,
    COMMIT = 2,
    ROLLBACK = 3,
    COMPENSATE = 4
};

#endif