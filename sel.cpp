// sel_vm_arena_word64_fold.cpp
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cctype>

using word64 = uint64_t;

// ------------------ Arena ------------------
class Arena {
public:
    explicit Arena(std::size_t chunk_size = 4096) : chunk_size_(chunk_size) {
        if (chunk_size_ == 0) fatal("(arena) chunk size cannot be zero");
        add_chunk(chunk_size_);
    }
    ~Arena() {
        Chunk* chunk = head_;
        while (chunk) {
            Chunk* next = chunk->next;
            std::free(chunk);
            chunk = next;
        }
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    template <typename T> T* alloc(std::size_t count = 1) noexcept {
        if (count > SIZE_MAX / sizeof(T)) fatal("(arena) allocation size overflow");
        return static_cast<T*>(alloc_aligned(sizeof(T) * count, alignof(T)));
    }

    char* strdup(const char* s) noexcept {
        if (!s) fatal("(arena) nullptr on strdup");
        std::size_t len = std::strlen(s) + 1;
        char* dst = alloc<char>(len);
        std::memcpy(dst, s, len);
        return dst;
    }

private:
    static constexpr std::size_t kDefaultChunkSize = 4096;
    struct Chunk { Chunk* next; std::byte data[1]; };
    Chunk* head_ = nullptr;
    Chunk* current_ = nullptr;
    std::size_t current_offset_ = 0;
    std::size_t chunk_size_;

    std::size_t align_up(std::size_t value, std::size_t align) noexcept {
        return (value + (align - 1)) & ~(align - 1);
    }

    void add_chunk(std::size_t min_size) noexcept {
        std::size_t size = (min_size > chunk_size_) ? min_size : chunk_size_;
        std::size_t total_size = sizeof(Chunk) - 1 + size;
        Chunk* chunk = static_cast<Chunk*>(std::malloc(total_size));
        if (!chunk) fatal("(arena) malloc failed for new chunk");
        chunk->next = nullptr;
        if (!head_) head_ = chunk;
        else current_->next = chunk;
        current_ = chunk;
        current_offset_ = 0;
    }

    void* alloc_aligned(std::size_t size, std::size_t align) noexcept {
        if ((align & (align - 1)) != 0) fatal("(arena) alignment must be power of two");
        std::size_t aligned_offset = align_up(current_offset_, align);
        if (aligned_offset + size > chunk_size_) {
            std::size_t new_chunk_size = (size > chunk_size_) ? size : chunk_size_;
            add_chunk(new_chunk_size);
            aligned_offset = 0;
        }
        void* ptr = current_->data + aligned_offset;
        current_offset_ = aligned_offset + size;
        return ptr;
    }

    [[noreturn]] void fatal(const char* msg) { std::cerr << msg << "\n"; std::exit(1); }
};

// ------------------ VM ------------------
typedef enum { OP_PUSH_INT, OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_SET_VAR } Sel_op;
struct Sel_inst { Sel_op op; word64 value; char* symbol_name = nullptr; };

struct Sel_vm {
    Sel_inst* code = nullptr;
    std::size_t size = 0;
    std::size_t ip = 0;

    word64* stack = nullptr;
    std::size_t sp = 0;
};

struct Sel_symbol { char* name; word64 value; };

struct Sel_symtab {
    Sel_symbol* data = nullptr;
    std::size_t count = 0;
};

word64 sym_lookup(Sel_symtab* st, const char* name) {
    for (std::size_t i = 0; i < st->count; i++)
        if (std::strcmp(st->data[i].name, name) == 0) return st->data[i].value;
    return 0;
}

void sym_set(Sel_symtab* st, Arena& arena, const char* name, word64 val) {
    for (std::size_t i = 0; i < st->count; i++) {
        if (std::strcmp(st->data[i].name, name) == 0) { st->data[i].value = val; return; }
    }
    Sel_symbol* s = &st->data[st->count++];
    s->name = arena.strdup(name);
    s->value = val;
}

// ------------------ Helpers ------------------
void skip_ws(const char** p) { while (**p && std::isspace(**p)) ++(*p); }
word64 parse_int(const char** p) { word64 v = 0; while (std::isdigit(**p)) { v = v * 10 + (**p - '0'); ++(*p); } return v; }
void emit(Sel_vm* vm, Sel_op op, word64 value = 0, char* sym = nullptr) { Sel_inst& i = vm->code[vm->size++]; i.op = op; i.value = value; i.symbol_name = sym; }

// Sentinel for "not a constant literal"
constexpr word64 NO_LITERAL = UINT64_MAX;

// ------------------ Compiler ------------------
word64 compile_expr(const char** p, Sel_vm* vm, Arena& arena, Sel_symtab* st);

char* parse_symbol(const char** p, Arena& arena) {
    skip_ws(p);
    const char* start = *p;
    while (**p && !std::isspace(**p) && **p != '(' && **p != ')') ++(*p);
    return arena.strdup(start);
}

// Map operator strings to enum
Sel_op map_op(const char* op) {
    if (std::strcmp(op, "+") == 0) return OP_PLUS;
    if (std::strcmp(op, "-") == 0) return OP_MINUS;
    if (std::strcmp(op, "*") == 0) return OP_MUL;
    if (std::strcmp(op, "/") == 0) return OP_DIV;
    return OP_PUSH_INT; // fallback
}

word64 compile_list(const char** p, Sel_vm* vm, Arena& arena, Sel_symtab* st) {
    skip_ws(p);
    char* op_str = parse_symbol(p, arena);
    Sel_op op = map_op(op_str);

    bool all_literals = true;
    word64 acc = 0;
    bool first = true;

    skip_ws(p);
    while (**p && **p != ')') {
        word64 val = compile_expr(p, vm, arena, st);
        if (val == NO_LITERAL) all_literals = false;

        if (first) { acc = val; first = false; }
        else if (all_literals) {
            switch(op) {
                case OP_PLUS: acc += val; break;
                case OP_MINUS: acc -= val; break;
                case OP_MUL: acc *= val; break;
                case OP_DIV: acc /= val; break;
                default: break;
            }
        } else {
            if (val != NO_LITERAL) emit(vm, OP_PUSH_INT, val);
            emit(vm, op);
            acc = NO_LITERAL;
        }
        skip_ws(p);
    }
    ++(*p); // consume ')'
    if (all_literals && acc != NO_LITERAL) emit(vm, OP_PUSH_INT, acc);
    return all_literals ? acc : NO_LITERAL;
}

word64 compile_expr(const char** p, Sel_vm* vm, Arena& arena, Sel_symtab* st) {
    skip_ws(p);
    if (**p == '(') {
        ++(*p);
        skip_ws(p);
        char* first_sym = parse_symbol(p, arena);

        if (std::strcmp(first_sym, "set") == 0) {
            char* sym_name = parse_symbol(p, arena);
            word64 val = compile_expr(p, vm, arena, st);
            emit(vm, OP_SET_VAR, val, sym_name);
            sym_set(st, arena, sym_name, val);
            skip_ws(p);
            if (**p == ')') ++(*p);
            return NO_LITERAL;
        } else {
            *p = *p - std::strlen(first_sym); // reset to reparse
            return compile_list(p, vm, arena, st);
        }
    }

    if (std::isdigit(**p)) return parse_int(p);

    // symbol
    char* sym_name = parse_symbol(p, arena);
    word64 val = sym_lookup(st, sym_name);
    emit(vm, OP_PUSH_INT, val);
    return NO_LITERAL;
}

// ------------------ VM Execution ------------------
word64 run(Sel_vm* vm) {
    vm->sp = 0;
    for (vm->ip = 0; vm->ip < vm->size; ++vm->ip) {
        Sel_inst& i = vm->code[vm->ip];
        switch (i.op) {
            case OP_PUSH_INT: vm->stack[vm->sp++] = i.value; break;
            case OP_PLUS: { word64 b = vm->stack[--vm->sp]; word64 a = vm->stack[--vm->sp]; vm->stack[vm->sp++] = a + b; break; }
            case OP_MINUS: { word64 b = vm->stack[--vm->sp]; word64 a = vm->stack[--vm->sp]; vm->stack[vm->sp++] = a - b; break; }
            case OP_MUL: { word64 b = vm->stack[--vm->sp]; word64 a = vm->stack[--vm->sp]; vm->stack[vm->sp++] = a * b; break; }
            case OP_DIV: { word64 b = vm->stack[--vm->sp]; word64 a = vm->stack[--vm->sp]; vm->stack[vm->sp++] = a / b; break; }
            case OP_SET_VAR: { word64 val = vm->stack[--vm->sp]; vm->stack[vm->sp++] = val; break; }
        }
    }
    return vm->stack[0];
}

// ------------------ REPL ------------------
void repl(Arena& arena) {
    Sel_vm vm{};
    vm.code = arena.alloc<Sel_inst>(1024);
    vm.stack = arena.alloc<word64>(1024);

    Sel_symtab symtab{};
    symtab.data = arena.alloc<Sel_symbol>(128);
    symtab.count = 0;

    std::string line;
    while (true) {
        std::cout << "sel> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        const char* p = line.c_str();
        vm.size = 0;
        compile_expr(&p, &vm, arena, &symtab);
        word64 result = run(&vm);
        std::cout << result << "\n";
    }
}

// ------------------ Main ------------------
int main() {
    Arena arena;
    std::cout << "Mini SEL VM (arena-only, word64, compile-time folding)\n";
    repl(arena);
}
