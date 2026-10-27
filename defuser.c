#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

/* 第 1 关：p1_compute 依次调用 p1_step2、p1_step0、p1_step1 */
int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t lo = (uint32_t)challenge;
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t step2 = (3 * hi) ^ (lo + 0xf7d1);
    uint32_t step0 = ((uint32_t)(challenge >> 16) ^ 0x5c70) + 4 * lo;
    values[0] = (3 * step0 + step2) ^ 0x4147;
    return write_answer(out, cap, values, count);
}

/* 第 2 关：6 个数，后一个由前一个递推 */
int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;
    values[0] = 3 * (uint32_t)((challenge >> 23) & 0xff);
    for (uint32_t i = 1; i < 6; ++i) {
        uint32_t bits = (uint32_t)(challenge >> (i + 12)) & 0x3f;
        values[i] = 3 * ((bits + i) ^ values[i - 1]);
    }
    return write_answer(out, cap, values, count);
}

/* 第 3 关：第一个数是跳转表下标，第二个数是对应分支的结果 */
int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    uint32_t index = (uint32_t)challenge & 7;
    uint32_t v = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3);
    uint32_t result = 0;
    /* 跳转表 0x405820 里下标和 p3_case_N 的对应关系是打乱的 */
    switch (index) {
    case 0: result = (2 * v) ^ 0x90eb; break;          /* p3_case_3 */
    case 1: result = 5 * v + 0x2779a; break;           /* p3_case_2 */
    case 2: result = (4 * v) ^ 0xc298; break;          /* p3_case_6 */
    case 3: result = 3 * v + 0xbe13; break;            /* p3_case_4 */
    case 4: result = (v ^ 0x7122) + (v >> 2); break;   /* p3_case_7 */
    case 5: result = 5 * v + 0x3f25f; break;           /* p3_case_5 */
    case 6: result = (v ^ 0x1785) + (v >> 2); break;   /* p3_case_1 */
    case 7: result = 5 * v + 0x6fcd; break;            /* p3_case_0 */
    }
    values[0] = index;
    values[1] = result;
    return write_answer(out, cap, values, count);
}

/* p4_walk：在 [lo, hi] 里二分查找 target，返回时逐层混合 */
static uint32_t p4_walk(uint32_t target, uint32_t lo, uint32_t hi, uint32_t key) {
    uint32_t mid = lo + ((hi - lo) >> 1);
    if (mid == target) return (mid + 0x7f) ^ key;
    if (target < mid) return ((3 * p4_walk(target, lo, mid - 1, key)) ^ (mid + key)) ^ 0x7;
    return ((3 * p4_walk(target, mid + 1, hi, key)) ^ (mid + key)) ^ 0x1d;
}

/* 第 4 关 */
int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t target = (uint32_t)challenge & 0x7f;
    uint32_t key = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = p4_walk(target, 0, 0x7f, key);
    return write_answer(out, cap, values, count);
}

/* 第 5 关的链表结点：偏移 0 是 next，偏移 8 和 0xc 各是一个 32 位数 */
struct p5_node {
    const struct p5_node *next;
    uint32_t value;
    uint32_t weight;
};

/* p5_nodes（0x405ba0 起，每个 16 字节）和 p5_heads（0x405b60）的内容 */
static const struct p5_node p5_nodes[12] = {
    {&p5_nodes[8], 0x664, 0x17f6},
    {&p5_nodes[2], 0x481, 0x34f2},
    {&p5_nodes[9], 0xb43, 0xa93f},
    {&p5_nodes[11], 0x830, 0x7ace},
    {&p5_nodes[7], 0x56c, 0x1121},
    {&p5_nodes[4], 0x77b, 0x4c55},
    {&p5_nodes[3], 0xa1c, 0xc66a},
    {&p5_nodes[1], 0x2f, 0xf51},
    {NULL, 0x989, 0xf31f},
    {&p5_nodes[0], 0x335, 0xef7e},
    {&p5_nodes[5], 0x261, 0xf98},
    {&p5_nodes[10], 0x158, 0xa155},
};

static const struct p5_node *const p5_heads[8] = {
    &p5_nodes[6], &p5_nodes[3], &p5_nodes[11], &p5_nodes[10],
    &p5_nodes[5], &p5_nodes[4], &p5_nodes[7], &p5_nodes[1],
};

/* 第 5 关：从 p5_heads[c & 7] 出发，再往后走 (c >> 3) & 3 步 */
int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    const struct p5_node *node = p5_heads[challenge & 7];
    uint32_t steps = (uint32_t)(challenge >> 3) & 3;
    uint32_t acc = ((uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32)) ^ 0x488a;
    acc = (3 * acc + node->weight) ^ node->value;
    for (uint32_t i = 0; i < steps; ++i) {
        node = node->next;
        acc = (3 * acc + node->weight) ^ node->value;
    }
    values[0] = acc;
    values[1] = node->value;
    return write_answer(out, cap, values, count);
}

/* 第 6 关的 6 个操作函数 p6_op_0 至 p6_op_5 */
static uint32_t p6_op_0(uint32_t v) { return (v ^ 0x81a5) + 2 * v; }
static uint32_t p6_op_1(uint32_t v) { return (32 * v + 0x8dc9) ^ (7 * v); }
static uint32_t p6_op_2(uint32_t v) { return (v + 0xc26) ^ (v >> 1); }
static uint32_t p6_op_3(uint32_t v) { return ((v << 3) | (v >> 29)) ^ 0x5beb; }
static uint32_t p6_op_4(uint32_t v) { return (v >> 2) | (v << 30); }
static uint32_t p6_op_5(uint32_t v) { return (v ^ (v >> 3)) + 0x3459; }

/* 函数指针表 p6_ops（0x405e00）的 8 项 */
static uint32_t (*const p6_ops[8])(uint32_t) = {
    p6_op_1, p6_op_0, p6_op_5, p6_op_5, p6_op_0, p6_op_4, p6_op_3, p6_op_2,
};

/* 第 6 关：先调 p6_ops[c & 7]，再调 p6_ops[(c >> 3) & 7] */
int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t v = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t first = p6_ops[challenge & 7](v) ^ 0x2f31;
    values[0] = p6_ops[(challenge >> 3) & 7](first) + 0x76a3;
    return write_answer(out, cap, values, count);
}

/*
 * Honor：secret_gate 把整行复制进栈上的结构体，偏移 0x20 处的 4 字节要是 "OPEN"，
 * 从偏移 0x24 开始的部分交给 secret_phase，要求是 3 个整数。
 * 前 32 个字符只用来填满第一个字段，内容不限。
 */
int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t c_fold = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t x_fold = (uint32_t)(context >> 32) ^ (uint32_t)context;
    uint32_t first = (uint32_t)context & 3;
    uint32_t mask = (first * 0x5f8) ^ (uint32_t)(context >> 16);
    uint32_t mixed = c_fold ^ x_fold;
    uint32_t second = (((mixed >> 3) ^ 0xe3c8) + (mixed ^ 0x5f8)) ^ mask;
    uint32_t third = second ^ (x_fold + first + 0xe3c8 + (c_fold >> 3));
    int size = snprintf(out, cap, "%s%s %" PRIu32 " %" PRIu32 " %" PRIu32,
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", "OPEN", first, second, third);
    return size < 0 || (size_t)size >= cap;
}
