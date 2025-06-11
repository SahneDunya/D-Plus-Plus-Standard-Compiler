#ifndef DPP_COMPILER_PARSER_AST_H
#define DPP_COMPILER_PARSER_AST_H

#include "../lexer/Token.h" // SourceLocation için
#include <vector>
#include <string>
#include <memory> // std::unique_ptr için
#include <utility> // std::move için
#include <iostream> // Debug için

namespace dppc {
namespace ast {

using SourceLocation = lexer::SourceLocation; // Kısaltma

// AST Düğümleri İçin Temel Sınıf
struct ASTNode {
    SourceLocation loc; // Kaynak kodundaki konumu

    ASTNode(SourceLocation l) : loc(std::move(l)) {}

    // Sanal yıkıcı, kalıtım için gerekli
    virtual ~ASTNode() = default;

    // Debug için AST düğümünü yazdırma (isteğe bağlı ama faydalı)
    virtual void dump(int indent = 0) const;

protected:
    // İndentasyonu yazdırmak için yardımcı
    void printIndent(int indent) const {
        for (int i = 0; i < indent; ++i) std::cerr << "  ";
    }
};

// --- İfadeler (Expressions) ---

// Tüm ifade düğümleri için temel sınıf
struct ExprAST : public ASTNode {
    ExprAST(SourceLocation l) : ASTNode(std::move(l)) {}
};

// Tamsayı Literal İfade
struct IntegerExprAST : public ExprAST {
    long long Value; // Sayı değeri

    IntegerExprAST(long long val, SourceLocation l)
        : ExprAST(std::move(l)), Value(val) {}

    void dump(int indent = 0) const override;
};

// Kayan Noktalı Literal İfade
struct FloatingExprAST : public ExprAST {
    double Value; // Sayı değeri

    FloatingExprAST(double val, SourceLocation l)
        : ExprAST(std::move(l)), Value(val) {}

    void dump(int indent = 0) const override;
};

// String Literal İfade
struct StringExprAST : public ExprAST {
    std::string Value;

    StringExprAST(std::string val, SourceLocation l)
        : ExprAST(std::move(l)), Value(std::move(val)) {}

    void dump(int indent = 0) const override;
};

// Boolean Literal İfade (true/false)
struct BooleanExprAST : public ExprAST {
    bool Value;

    BooleanExprAST(bool val, SourceLocation l)
        : ExprAST(std::move(l)), Value(val) {}

    void dump(int indent = 0) const override;
};

// Identifier (Değişken/Fonksiyon Adı) İfade
struct IdentifierExprAST : public ExprAST {
    std::string Name;

    IdentifierExprAST(std::string name, SourceLocation l)
        : ExprAST(std::move(l)), Name(std::move(name)) {}

    void dump(int indent = 0) const override;
};

// İkili Operatör İfade (örn: a + b, x == y)
struct BinaryOpExprAST : public ExprAST {
    lexer::TokenType Op; // Operatör token türü (+, -, * vb.)
    std::unique_ptr<ExprAST> Left; // Sol operand
    std::unique_ptr<ExprAST> Right; // Sağ operand

    BinaryOpExprAST(lexer::TokenType op, std::unique_ptr<ExprAST> left, std::unique_ptr<ExprAST> right, SourceLocation l)
        : ExprAST(std::move(l)), Op(op), Left(std::move(left)), Right(std::move(right)) {}

    void dump(int indent = 0) const override;
};

// Tekli Operatör İfade (örn: -a, !b, *ptr, &val)
struct UnaryOpExprAST : public ExprAST {
    lexer::TokenType Op; // Operatör token türü (-, !, *, & vb.)
    std::unique_ptr<ExprAST> Operand; // Operand

    UnaryOpExprAST(lexer::TokenType op, std::unique_ptr<ExprAST> operand, SourceLocation l)
        : ExprAST(std::move(l)), Op(op), Operand(std::move(operand)) {}

    void dump(int indent = 0) const override;
};


// Fonksiyon Çağrısı İfade (örn: f(x, y))
struct CallExprAST : public ExprAST {
    std::string CalleeName; // Çağrılan fonksiyonun adı (şimdilik basit tutalım)
    std::vector<std::unique_ptr<ExprAST>> Args; // Argüman listesi

    CallExprAST(std::string callee, std::vector<std::unique_ptr<ExprAST>> args, SourceLocation l)
        : ExprAST(std::move(l)), CalleeName(std::move(callee)), Args(std::move(args)) {}

    void dump(int indent = 0) const override;
};

// Üye Erişimi İfade (örn: obj.member, ptr->member)
struct MemberAccessExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Base; // Erişilen nesne/pointer ifadesi
    std::string MemberName;       // Erişilen üyenin adı
    bool IsPointerAccess;         // -> operatörü mü kullanıldı?

    MemberAccessExprAST(std::unique_ptr<ExprAST> base, std::string memberName, bool isPtrAccess, SourceLocation l)
        : ExprAST(std::move(l)), Base(std::move(base)), MemberName(std::move(memberName)), IsPointerAccess(isPtrAccess) {}

    void dump(int indent = 0) const override;
};

// Diz (Array) Erişimi İfade (örn: arr[index])
struct ArrayAccessExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Base; // Erişilen dizi ifadesi
    std::unique_ptr<ExprAST> Index; // İndeks ifadesi

    ArrayAccessExprAST(std::unique_ptr<ExprAST> base, std::unique_ptr<ExprAST> index, SourceLocation l)
        : ExprAST(std::move(l)), Base(std::move(base)), Index(std::move(index)) {}

    void dump(int indent = 0) const override;
};

// Yeni Nesne Oluşturma İfadesi (örn: new MyStruct(args))
struct NewExprAST : public ExprAST {
    std::string TypeName; // Oluşturulan nesnenin tipi (şimdilik string tutalım)
    std::vector<std::unique_ptr<ExprAST>> Args; // Constructor argümanları

    NewExprAST(std::string typeName, std::vector<std::unique_ptr<ExprAST>> args, SourceLocation l)
        : ExprAST(std::move(l)), TypeName(std::move(typeName)), Args(std::move(args)) {}

    void dump(int indent = 0) const override;
};

// D++'a Özgü İfadeler

// Ödünç Alma (Borrow) İfade (örn: &var, &mut var)
struct BorrowExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Operand; // Ödünç alınan operand
    bool IsMutable;                   // mutable(&mut) ödünç alma mı?

    BorrowExprAST(std::unique_ptr<ExprAST> operand, bool isMut, SourceLocation l)
        : ExprAST(std::move(l)), Operand(std::move(operand)), IsMutable(isMut) {}

    void dump(int indent = 0) const override;
};

// Dereference İfade (örn: *ptr)
struct DereferenceExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Operand; // Dereference edilecek operand

    DereferenceExprAST(std::unique_ptr<ExprAST> operand, SourceLocation l)
        : ExprAST(std::move(l)), Operand(std::move(operand)) {}

    void dump(int indent = 0) const override;
};

// Match İfadesi (örn: match value { pattern => expr, ... })
struct MatchExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Value; // Match yapılan ifade

    // Match kolları (Pattern -> Expression)
    struct MatchArm {
        std::unique_ptr<struct PatternAST> Pattern; // Eşleşme paterni
        std::unique_ptr<ExprAST> Guard;             // Opsiyonel guard ifadesi (if condition)
        std::unique_ptr<ExprAST> Body;              // Eşleştiğinde çalışacak ifade

        MatchArm(std::unique_ptr<struct PatternAST> pattern, std::unique_ptr<ExprAST> guard, std::unique_ptr<ExprAST> body)
            : Pattern(std::move(pattern)), Guard(std::move(guard)), Body(std::move(body)) {}
         // Yıkıcı std::unique_ptr sayesinde otomatik yönetilir
    };

    std::vector<MatchArm> Arms; // Match kolları listesi

    MatchExprAST(std::unique_ptr<ExprAST> value, std::vector<MatchArm> arms, SourceLocation l)
        : ExprAST(std::move(l)), Value(std::move(value)), Arms(std::move(arms)) {}

    void dump(int indent = 0) const override;
};


// --- Desenler (Patterns) (Match ifadeleri için) ---

// Tüm pattern düğümleri için temel sınıf
struct PatternAST : public ASTNode {
    PatternAST(SourceLocation l) : ASTNode(std::move(l)) {}
};

// Wildcard Pattern (_)
struct WildcardPatternAST : public PatternAST {
    WildcardPatternAST(SourceLocation l) : PatternAST(std::move(l)) {}
    void dump(int indent = 0) const override;
};

// Identifier Pattern (x, mut x)
struct IdentifierPatternAST : public PatternAST {
    std::string Name;
    bool IsMutable; // `mut` kullanıldı mı?

    IdentifierPatternAST(std::string name, bool isMut, SourceLocation l)
        : PatternAST(std::move(l)), Name(std::move(name)), IsMutable(isMut) {}
    void dump(int indent = 0) const override;
};

// Literal Pattern (1, "hello", true)
struct LiteralPatternAST : public PatternAST {
    std::unique_ptr<ExprAST> Literal; // Literal ifade düğümü (Integer, String, Boolean vb.)

    LiteralPatternAST(std::unique_ptr<ExprAST> literal, SourceLocation l)
        : PatternAST(std::move(l)), Literal(std::move(literal)) {}
    void dump(int indent = 0) const override;
};

// Yapı (Struct) Deconstruction Pattern (Point { x, y: new_y })
struct StructDeconstructionPatternAST : public PatternAST {
    std::string TypeName; // Yapı tipi adı
    // Alan eşleşmeleri (alan adı -> pattern)
    struct FieldMatch {
        std::string FieldName;
        std::unique_ptr<PatternAST> Binding; // Alanın eşleştiği pattern

        FieldMatch(std::string fieldName, std::unique_ptr<PatternAST> binding)
            : FieldName(std::move(fieldName)), Binding(std::move(binding)) {}
    };
    std::vector<FieldMatch> Fields; // Alan eşleşmeleri listesi
    bool IgnoreExtraFields; // `..` kullanıldı mı?

    StructDeconstructionPatternAST(std::string typeName, std::vector<FieldMatch> fields, bool ignoreExtra, SourceLocation l)
        : PatternAST(std::move(l)), TypeName(std::move(typeName)), Fields(std::move(fields)), IgnoreExtraFields(ignoreExtra) {}
     void dump(int indent = 0) const override;
};

// Enum Variant Pattern (Option::Some(value), Result::Ok(val))
struct EnumVariantPatternAST : public PatternAST {
    std::string EnumName; // Enum adı (örn: Option, Result)
    std::string VariantName; // Variant adı (örn: Some, Ok)
    std::vector<std::unique_ptr<PatternAST>> Args; // Variant argümanları (tuple pattern gibi)

    EnumVariantPatternAST(std::string enumName, std::string variantName, std::vector<std::unique_ptr<PatternAST>> args, SourceLocation l)
        : PatternAST(std::move(l)), EnumName(std::move(enumName)), VariantName(std::move(variantName)), Args(std::move(args)) {}
    void dump(int indent = 0) const override;
};

// Tuple Pattern ((a, b, _))
struct TuplePatternAST : public PatternAST {
    std::vector<std::unique_ptr<PatternAST>> Elements;

    TuplePatternAST(std::vector<std::unique_ptr<PatternAST>> elements, SourceLocation l)
        : PatternAST(std::move(l)), Elements(std::move(elements)) {}
    void dump(int indent = 0) const override;
};

// Reference Pattern (&pat, &mut pat)
struct ReferencePatternAST : public PatternAST {
    std::unique_ptr<PatternAST> SubPattern;
    bool IsMutable;

    ReferencePatternAST(std::unique_ptr<PatternAST> subPattern, bool isMut, SourceLocation l)
        : PatternAST(std::move(l)), SubPattern(std::move(subPattern)), IsMutable(isMut) {}
     void dump(int indent = 0) const override;
};

// Range Pattern (1..=5, 'a'..'z')
// TODO: Range pattern desteği eklenebilir (Integer veya Character range)
 struct RangePatternAST : public PatternAST { ... };


// --- Deyimler (Statements) ---

// Tüm deyim düğümleri için temel sınıf
struct StmtAST : public ASTNode {
    StmtAST(SourceLocation l) : ASTNode(std::move(l)) {}
};

// İfade Deyimi (sonunda noktalı virgül olan ifade)
struct ExprStmtAST : public StmtAST {
    std::unique_ptr<ExprAST> Expr;

    ExprStmtAST(std::unique_ptr<ExprAST> expr, SourceLocation l)
        : StmtAST(std::move(l)), Expr(std::move(expr)) {}

    void dump(int indent = 0) const override;
};

// Blok Deyimi ({ ... })
struct BlockStmtAST : public StmtAST {
    std::vector<std::unique_ptr<StmtAST>> Statements; // Deyim listesi

    BlockStmtAST(std::vector<std::unique_ptr<StmtAST>> statements, SourceLocation l)
        : StmtAST(std::move(l)), Statements(std::move(statements)) {}

    void dump(int indent = 0) const override;
};

// If Deyimi
struct IfStmtAST : public StmtAST {
    std::unique_ptr<ExprAST> Condition; // Koşul ifadesi
    std::unique_ptr<StmtAST> ThenBlock; // If doğru ise çalışacak blok
    std::unique_ptr<StmtAST> ElseBlock; // İsteğe bağlı Else/Else If blok (null olabilir)

    IfStmtAST(std::unique_ptr<ExprAST> condition, std::unique_ptr<StmtAST> thenBlock, std::unique_ptr<StmtAST> elseBlock, SourceLocation l)
        : StmtAST(std::move(l)), Condition(std::move(condition)), ThenBlock(std::move(thenBlock)), ElseBlock(std::move(elseBlock)) {}

    void dump(int indent = 0) const override;
};

// While Döngüsü Deyimi
struct WhileStmtAST : public StmtAST {
    std::unique_ptr<ExprAST> Condition; // Koşul ifadesi
    std::unique_ptr<StmtAST> Body;      // Döngü gövdesi

    WhileStmtAST(std::unique_ptr<ExprAST> condition, std::unique_ptr<StmtAST> body, SourceLocation l)
        : StmtAST(std::move(l)), Condition(std::move(condition)), Body(std::move(body)) {}

    void dump(int indent = 0) const override;
};

// For Döngüsü Deyimi (Basit range tabanlı veya C-stili olabilir)
// D'deki 'for' ve 'foreach' farklı AST düğümleri gerektirebilir.
// Basit C-stili for:
struct ForStmtAST : public StmtAST {
    std::unique_ptr<StmtAST> Init;      // İsteğe bağlı başlangıç deyimi
    std::unique_ptr<ExprAST> Condition; // İsteğe bağlı koşul ifadesi
    std::unique_ptr<ExprAST> LoopEnd;   // İsteğe bağlı döngü sonu ifadesi
    std::unique_ptr<StmtAST> Body;      // Döngü gövdesi

    ForStmtAST(std::unique_ptr<StmtAST> init, std::unique_ptr<ExprAST> condition, std::unique_ptr<ExprAST> loopEnd, std::unique_ptr<StmtAST> body, SourceLocation l)
        : StmtAST(std::move(l)), Init(std::move(init)), Condition(std::move(condition)), LoopEnd(std::move(loopEnd)), Body(std::move(body)) {}

    void dump(int indent = 0) const override;
};

// Return Deyimi
struct ReturnStmtAST : public StmtAST {
    std::unique_ptr<ExprAST> ReturnValue; // İsteğe bağlı dönüş değeri ifadesi

    ReturnStmtAST(std::unique_ptr<ExprAST> returnValue, SourceLocation l)
        : StmtAST(std::move(l)), ReturnValue(std::move(returnValue)) {}

    void dump(int indent = 0) const override;
};

// Break Deyimi
struct BreakStmtAST : public StmtAST {
     // İsteğe bağlı label eklenebilir
    BreakStmtAST(SourceLocation l) : StmtAST(std::move(l)) {}
    void dump(int indent = 0) const override;
};

// Continue Deyimi
struct ContinueStmtAST : public StmtAST {
     // İsteğe bağlı label eklenebilir
    ContinueStmtAST(SourceLocation l) : StmtAST(std::move(l)) {}
     void dump(int indent = 0) const override;
};


// D++'a Özgü Deyimler

// Let Deyimi (Değişken Tanımlama ve Atama)
struct LetStmtAST : public StmtAST {
    std::unique_ptr<PatternAST> Pattern; // Eşleşme paterni (genellikle IdentifierPattern)
     std::string VarName; // Değişken adı (Pattern içinde de olabilir ama basitlik için burada tutulabilir)
     bool IsMutable;    // `mut` anahtar kelimesi kullanıldı mı?
    std::unique_ptr<struct TypeAST> Type; // İsteğe bağlı tip belirtimi
    std::unique_ptr<ExprAST> Initializer; // İsteğe bağlı başlangıç değeri ataması

    LetStmtAST(std::unique_ptr<PatternAST> pattern, std::unique_ptr<TypeAST> type, std::unique_ptr<ExprAST> initializer, SourceLocation l)
        : StmtAST(std::move(l)), Pattern(std::move(pattern)), Type(std::move(type)), Initializer(std::move(initializer)) {}

    void dump(int indent = 0) const override;
};


// --- Bildirimler (Declarations) ---

// Tüm bildirim düğümleri için temel sınıf
struct DeclAST : public ASTNode {
    DeclAST(SourceLocation l) : ASTNode(std::move(l)) {}
};

// Fonksiyon Parametresi
struct ParameterAST : public ASTNode {
    std::string Name;
    std::unique_ptr<struct TypeAST> Type;
    // bool IsMutable; // Parametre mutable mi? (D++'a özgü olabilir)

    ParameterAST(std::string name, std::unique_ptr<TypeAST> type, SourceLocation l)
        : ASTNode(std::move(l)), Name(std::move(name)), Type(std::move(type)) {}

    void dump(int indent = 0) const override;
};

// Fonksiyon Bildirimi/Tanımı (Function Declaration/Definition)
struct FunctionDeclAST : public DeclAST {
    std::string Name;
    std::vector<std::unique_ptr<ParameterAST>> Params; // Parametre listesi
    std::unique_ptr<struct TypeAST> ReturnType;     // Dönüş tipi
    std::unique_ptr<BlockStmtAST> Body;             // Fonksiyon gövdesi (tanım için, bildirimde null)

    FunctionDeclAST(std::string name, std::vector<std::unique_ptr<ParameterAST>> params, std::unique_ptr<TypeAST> returnType, std::unique_ptr<BlockStmtAST> body, SourceLocation l)
        : DeclAST(std::move(l)), Name(std::move(name)), Params(std::move(params)), ReturnType(std::move(returnType)), Body(std::move(body)) {}

    void dump(int indent = 0) const override;
};

// Yapı (Struct) Üyesi (Alan)
struct StructFieldAST : public ASTNode {
    std::string Name;
    std::unique_ptr<struct TypeAST> Type;
    // İsteğe bağlı başlangıç değeri? D standardına göre.

    StructFieldAST(std::string name, std::unique_ptr<TypeAST> type, SourceLocation l)
        : ASTNode(std::move(l)), Name(std::move(name)), Type(std::move(type)) {}

    void dump(int indent = 0) const override;
};

// Yapı (Struct) Bildirimi
struct StructDeclAST : public DeclAST {
    std::string Name;
    std::vector<std::unique_ptr<StructFieldAST>> Fields; // Alan listesi
    // D'deki struct özellikleri (union, class, interface, invariant vb.) buraya eklenebilir.

    StructDeclAST(std::string name, std::vector<std::unique_ptr<StructFieldAST>> fields, SourceLocation l)
        : DeclAST(std::move(l)), Name(std::move(name)), Fields(std::move(fields)) {}

     void dump(int indent = 0) const override;
};

// Trait Bildirimi (D++'a özgü)
struct TraitDeclAST : public DeclAST {
    std::string Name;
    std::vector<std::unique_ptr<FunctionDeclAST>> RequiredFunctions; // Trait'in gerektirdiği fonksiyon bildirimleri
    // Trait'lerin gerektirdiği tipler veya diğer üyeler de olabilir.

    TraitDeclAST(std::string name, std::vector<std::unique_ptr<FunctionDeclAST>> requiredFuncs, SourceLocation l)
        : DeclAST(std::move(l)), Name(std::move(name)), RequiredFunctions(std::move(requiredFuncs)) {}

    void dump(int indent = 0) const override;
};

// Implementasyon (Impl) Bildirimi (D++'a özgü)
struct ImplDeclAST : public DeclAST {
    // Trait implementasyonu için:
    std::string TraitName; // Implemente edilen Trait adı (trait implementasyonu ise)
    std::string ForTypeName; // Hangi tip için implementasyon yapılıyor
    // TraitName boşsa, bu tip için inherent implementasyonlardır.

    std::vector<std::unique_ptr<FunctionDeclAST>> Methods; // Implemente edilen metodlar
    // Diğer üyeler (trait'ten gelen veya inherent) buraya eklenebilir.

    ImplDeclAST(std::string traitName, std::string forType, std::vector<std::unique_ptr<FunctionDeclAST>> methods, SourceLocation l)
        : DeclAST(std::move(l)), TraitName(std::move(traitName)), ForTypeName(std::move(forType)), Methods(std::move(methods)) {}

    void dump(int indent = 0) const override;
};


// Enum Variant (Enum üyesi)
struct EnumVariantAST : public ASTNode {
     std::string Name;
     // İsteğe bağlı değer (D'deki gibi) veya ilişkili tipler (Rust benzeri tuple/struct variant)
      std::unique_ptr<ExprAST> Value; // D stili value (Enum.Member = 5)
      std::vector<std::unique_ptr<TypeAST>> AssociatedTypes; // Rust stili tuple variant (Variant(i32, String))
      std::vector<std::unique_ptr<StructFieldAST>> AssociatedFields; // Rust stili struct variant (Variant { x: i32 })

    // Şimdilik sadece isme sahip basit enum varyantları tanımlayalım
     EnumVariantAST(std::string name, SourceLocation l) : ASTNode(std::move(l)), Name(std::move(name)) {}
     void dump(int indent = 0) const override;
};

// Enum Bildirimi
struct EnumDeclAST : public DeclAST {
    std::string Name;
    std::vector<std::unique_ptr<EnumVariantAST>> Variants; // Varyant listesi

    EnumDeclAST(std::string name, std::vector<std::unique_ptr<EnumVariantAST>> variants, SourceLocation l)
        : DeclAST(std::move(l)), Name(std::move(name)), Variants(std::move(variants)) {}

    void dump(int indent = 0) const override;
};


// --- Tipler (Types) ---

// Tip düğümleri için temel sınıf
// Not: Tip sisteminin kendisi genellikle Semantic Analiz aşamasında yönetilir,
// ancak Parser dilin sözdiziminde görünen tipleri temsil eden AST düğümleri oluşturur.
struct TypeAST : public ASTNode {
    TypeAST(SourceLocation l) : ASTNode(std::move(l)) {}
    virtual ~TypeAST() = default; // Sanal yıkıcı

    void dump(int indent = 0) const override; // Genel dump implementasyonu
};

// Temel Tip (int, float, bool, void vb.)
struct PrimitiveTypeAST : public TypeAST {
    lexer::TokenType TypeToken; // int, float, bool, void gibi token türü

    PrimitiveTypeAST(lexer::TokenType typeToken, SourceLocation l)
        : TypeAST(std::move(l)), TypeToken(typeToken) {}
    void dump(int indent = 0) const override;
};

// Identifier Tip (Kullanıcı tanımlı tipler: MyStruct, MyClass, MyTrait)
struct IdentifierTypeAST : public TypeAST {
    std::string Name;

    IdentifierTypeAST(std::string name, SourceLocation l)
        : TypeAST(std::move(l)), Name(std::move(name)) {}
    void dump(int indent = 0) const override;
};

// Pointer Tip (*T)
struct PointerTypeAST : public TypeAST {
    std::unique_ptr<TypeAST> BaseType;

    PointerTypeAST(std::unique_ptr<TypeAST> baseType, SourceLocation l)
        : TypeAST(std::move(l)), BaseType(std::move(baseType)) {}
     void dump(int indent = 0) const override;
};

// Referans Tip (&T, &mut T)
struct ReferenceTypeAST : public TypeAST {
    std::unique_ptr<TypeAST> BaseType;
    bool IsMutable;

    ReferenceTypeAST(std::unique_ptr<TypeAST> baseType, bool isMut, SourceLocation l)
        : TypeAST(std::move(l)), BaseType(std::move(baseType)), IsMutable(isMut) {}
    void dump(int indent = 0) const override;
};

// Dizi (Array) Tip ([T; N] veya [T])
struct ArrayTypeAST : public TypeAST {
    std::unique_ptr<TypeAST> ElementType;
    std::unique_ptr<ExprAST> Size; // Boyut ifadesi (sabit olmalı, sema kontrol eder) - isteğe bağlı

    ArrayTypeAST(std::unique_ptr<TypeAST> elementType, std::unique_ptr<ExprAST> size, SourceLocation l)
        : TypeAST(std::move(l)), ElementType(std::move(elementType)), Size(std::move(size)) {}

     void dump(int indent = 0) const override;
};

// Tuple Tip ((T1, T2, ...))
struct TupleTypeAST : public TypeAST {
    std::vector<std::unique_ptr<TypeAST>> ElementTypes;

    TupleTypeAST(std::vector<std::unique_ptr<TypeAST>> elementTypes, SourceLocation l)
        : TypeAST(std::move(l)), ElementTypes(std::move(elementTypes)) {}
     void dump(int indent = 0) const override;
};

// Option<T> Tip (D++'a özgü)
struct OptionTypeAST : public TypeAST {
    std::unique_ptr<TypeAST> InnerType;

    OptionTypeAST(std::unique_ptr<TypeAST> innerType, SourceLocation l)
        : TypeAST(std::move(l)), InnerType(std::move(innerType)) {}
    void dump(int indent = 0) const override;
};

// Result<T, E> Tip (D++'a özgü)
struct ResultTypeAST : public TypeAST {
    std::unique_ptr<TypeAST> OkType; // Başarılı durumdaki değerin tipi
    std::unique_ptr<TypeAST> ErrType; // Hata durumundaki değerin tipi

    ResultTypeAST(std::unique_ptr<TypeAST> okType, std::unique_ptr<TypeAST> errType, SourceLocation l)
        : TypeAST(std::move(l)), OkType(std::move(okType)), ErrType(std::move(errType)) {}
    void dump(int indent = 0) const override;
};

// --- Kök Düğüm ---

// Kaynak Dosyanın Kök Düğümü
struct SourceFileAST : public ASTNode {
    std::string Filename;
    std::vector<std::unique_ptr<DeclAST>> Declarations; // Dosyadaki üst düzey bildirimler

    SourceFileAST(std::string filename, std::vector<std::unique_ptr<DeclAST>> declarations, SourceLocation l)
        : ASTNode(std::move(l)), Filename(std::move(filename)), Declarations(std::move(declarations)) {}

     void dump(int indent = 0) const override;
};


} // namespace ast
} // namespace dppc

#endif // DPP_COMPILER_PARSER_AST_H
