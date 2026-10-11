#pragma once
#include "common/source_location.h"
#include "sema/type.h"
#include <memory>
#include <string>
#include <vector>

namespace minic {

// Forward declarations
struct Expr;
struct Stmt;
struct ASTNode;
class ASTVisitor;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ─── Binary / Unary / Assignment operators ───────────────────────────
enum class BinOp {
    Add, Sub, Mul, Div, Mod,
    BitAnd, BitOr, BitXor, ShiftL, ShiftR,
    LogAnd, LogOr,
    Eq, NEq, Lt, Gt, LEq, GEq
};

enum class UnaryOp { Neg, LogNot, BitNot, Deref, AddrOf };

enum class AssignOp { Assign, AddAssign, SubAssign, MulAssign, DivAssign };

const char* binOpToString(BinOp op);
const char* unaryOpToString(UnaryOp op);

// ─── AST Base ────────────────────────────────────────────────────────
struct ASTNode {
    SourceLoc loc;
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& v) = 0;
};

// ─── Expression base ─────────────────────────────────────────────────
struct Expr : ASTNode {
    TypeInfo resolved_type; // filled in by semantic analysis
};

struct IntLiteralExpr : Expr {
    int64_t value;
    void accept(ASTVisitor& v) override;
};

struct CharLiteralExpr : Expr {
    char value;
    void accept(ASTVisitor& v) override;
};

struct StringLiteralExpr : Expr {
    std::string value;
    void accept(ASTVisitor& v) override;
};

struct IdentExpr : Expr {
    std::string name;
    void accept(ASTVisitor& v) override;
};

struct BinaryExpr : Expr {
    BinOp op;
    ExprPtr lhs, rhs;
    void accept(ASTVisitor& v) override;
};

struct UnaryExpr : Expr {
    UnaryOp op;
    ExprPtr operand;
    void accept(ASTVisitor& v) override;
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<ExprPtr> args;
    void accept(ASTVisitor& v) override;
};

struct ArraySubscriptExpr : Expr {
    ExprPtr array;
    ExprPtr index;
    void accept(ASTVisitor& v) override;
};

struct AssignExpr : Expr {
    AssignOp op;
    ExprPtr target;
    ExprPtr value;
    void accept(ASTVisitor& v) override;
};

struct CastExpr : Expr {
    TypeInfo target_type;
    ExprPtr operand;
    void accept(ASTVisitor& v) override;
};

struct SizeofExpr : Expr {
    TypeInfo query_type;
    void accept(ASTVisitor& v) override;
};

// ─── Statement base ──────────────────────────────────────────────────
struct Stmt : ASTNode {};

struct DeclStmt : Stmt {
    TypeInfo type;
    std::string name;
    ExprPtr init; // may be null
    bool is_array = false;
    int array_size = 0;
    void accept(ASTVisitor& v) override;
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    void accept(ASTVisitor& v) override;
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> stmts;
    void accept(ASTVisitor& v) override;
};

struct IfStmt : Stmt {
    ExprPtr cond;
    StmtPtr then_branch;
    StmtPtr else_branch; // may be null
    void accept(ASTVisitor& v) override;
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    StmtPtr body;
    void accept(ASTVisitor& v) override;
};

struct ForStmt : Stmt {
    StmtPtr init; // may be null
    ExprPtr cond; // may be null
    ExprPtr incr; // may be null
    StmtPtr body;
    void accept(ASTVisitor& v) override;
};

struct ReturnStmt : Stmt {
    ExprPtr value; // may be null for void functions
    void accept(ASTVisitor& v) override;
};

// ─── Top-level declarations ─────────────────────────────────────────
struct Param {
    TypeInfo type;
    std::string name;
    SourceLoc loc;
};

struct FunctionDef : ASTNode {
    TypeInfo return_type;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<BlockStmt> body; // null for forward declarations
    bool is_declaration = false;     // true if just a declaration (no body)
    void accept(ASTVisitor& v) override;
};

struct GlobalDecl : ASTNode {
    TypeInfo type;
    std::string name;
    ExprPtr init;
    bool is_array = false;
    int array_size = 0;
    void accept(ASTVisitor& v) override;
};

struct Program : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> decls;
    void accept(ASTVisitor& v) override;
};

// ─── Visitor pattern ─────────────────────────────────────────────────
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(IntLiteralExpr& node) = 0;
    virtual void visit(CharLiteralExpr& node) = 0;
    virtual void visit(StringLiteralExpr& node) = 0;
    virtual void visit(IdentExpr& node) = 0;
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;
    virtual void visit(ArraySubscriptExpr& node) = 0;
    virtual void visit(AssignExpr& node) = 0;
    virtual void visit(CastExpr& node) = 0;
    virtual void visit(SizeofExpr& node) = 0;
    virtual void visit(DeclStmt& node) = 0;
    virtual void visit(ExprStmt& node) = 0;
    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(ForStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(FunctionDef& node) = 0;
    virtual void visit(GlobalDecl& node) = 0;
    virtual void visit(Program& node) = 0;
};

// accept() implementations (inline)
inline void IntLiteralExpr::accept(ASTVisitor& v)     { v.visit(*this); }
inline void CharLiteralExpr::accept(ASTVisitor& v)    { v.visit(*this); }
inline void StringLiteralExpr::accept(ASTVisitor& v)  { v.visit(*this); }
inline void IdentExpr::accept(ASTVisitor& v)          { v.visit(*this); }
inline void BinaryExpr::accept(ASTVisitor& v)         { v.visit(*this); }
inline void UnaryExpr::accept(ASTVisitor& v)          { v.visit(*this); }
inline void CallExpr::accept(ASTVisitor& v)           { v.visit(*this); }
inline void ArraySubscriptExpr::accept(ASTVisitor& v) { v.visit(*this); }
inline void AssignExpr::accept(ASTVisitor& v)         { v.visit(*this); }
inline void CastExpr::accept(ASTVisitor& v)           { v.visit(*this); }
inline void SizeofExpr::accept(ASTVisitor& v)         { v.visit(*this); }
inline void DeclStmt::accept(ASTVisitor& v)           { v.visit(*this); }
inline void ExprStmt::accept(ASTVisitor& v)           { v.visit(*this); }
inline void BlockStmt::accept(ASTVisitor& v)          { v.visit(*this); }
inline void IfStmt::accept(ASTVisitor& v)             { v.visit(*this); }
inline void WhileStmt::accept(ASTVisitor& v)          { v.visit(*this); }
inline void ForStmt::accept(ASTVisitor& v)            { v.visit(*this); }
inline void ReturnStmt::accept(ASTVisitor& v)         { v.visit(*this); }
inline void FunctionDef::accept(ASTVisitor& v)        { v.visit(*this); }
inline void GlobalDecl::accept(ASTVisitor& v)         { v.visit(*this); }
inline void Program::accept(ASTVisitor& v)            { v.visit(*this); }

} // namespace minic
