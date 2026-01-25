#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <iostream>

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

    template <typename T>
    T* alloc(std::size_t count = 1) noexcept {
        if (count > SIZE_MAX / sizeof(T)) fatal("(arena) allocation overflow");
        return static_cast<T*>(alloc_aligned(sizeof(T) * count, alignof(T)));
    }

private:
    struct Chunk { Chunk* next; std::byte data[1]; };
    Chunk* head_ = nullptr;
    Chunk* current_ = nullptr;
    std::size_t current_offset_ = 0;
    std::size_t chunk_size_;

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
        std::size_t aligned_offset = (current_offset_ + (align - 1)) & ~(align - 1);
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
enum Sel_op { OP_PUSH_INT, OP_PLUS };
struct Sel_inst { Sel_op op; word64 value; };

struct Sel_vm {
    Sel_inst* code = nullptr;
    std::size_t size = 0;
    std::size_t ip = 0;

    word64* stack = nullptr;
    std::size_t sp = 0;
};

// ------------------ Helpers ------------------
void skip_ws(const char** p) { while (**p && std::isspace(**p)) ++(*p); }
word64 parse_int(const char** p) { word64 v = 0; while (std::isdigit(**p)) { v = v*10 + (**p-'0'); ++(*p); } return v; }
void emit(Sel_vm* vm, Sel_op op, word64 value = 0) { Sel_inst& i = vm->code[vm->size++]; i.op = op; i.value = value; }

// ------------------ Compiler ------------------
void compile_expr(const char** p, Sel_vm* vm, Arena& arena);

void compile_list(const char** p, Sel_vm* vm, Arena& arena) {
    skip_ws(p);
    if (**p != '+') { std::cerr << "Only + operator supported\n"; std::exit(1); }
    ++(*p); // consume '+'

    bool first_operand = true;

    skip_ws(p);
    while (**p && **p != ')') {
        compile_expr(p, vm, arena);
        if (!first_operand) emit(vm, OP_PLUS);
        first_operand = false;
        skip_ws(p);
    }

    ++(*p); // consume ')'
}

void compile_expr(const char** p, Sel_vm* vm, Arena& arena) {
    skip_ws(p);
    if (**p == '(') {
        ++(*p); // consume '('
        compile_list(p, vm, arena);
        return;
    }
    if (std::isdigit(**p)) {
        word64 v = parse_int(p);
        emit(vm, OP_PUSH_INT, v);
        return;
    }
    std::cerr << "Syntax error\n"; std::exit(1);
}

// ------------------ VM Execution ------------------
word64 run(Sel_vm* vm) {
    vm->sp = 0;
    for (vm->ip = 0; vm->ip < vm->size; ++vm->ip) {
        Sel_inst& i = vm->code[vm->ip];
        switch (i.op) {
            case OP_PUSH_INT: vm->stack[vm->sp++] = i.value; break;
            case OP_PLUS: {
                word64 b = vm->stack[--vm->sp];
                word64 a = vm->stack[--vm->sp];
                vm->stack[vm->sp++] = a + b;
                break;
            }
        }
    }
    return vm->stack[0];
}

// ------------------ Main ------------------
int main() {
    Arena arena;

    const char* exp = "(+ 10 2 (+ 1 5 2))"; // nested example
    const char* p = exp;

    Sel_vm vm{};
    vm.code = arena.alloc<Sel_inst>(64);
    vm.stack = arena.alloc<word64>(64);
    vm.size = 0;

    compile_expr(&p, &vm, arena);

    word64 result = run(&vm);
    std::cout << result << "\n"; // -> 20
}
