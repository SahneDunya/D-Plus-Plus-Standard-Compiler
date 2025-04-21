module dpp_types;

import std.string;

// D++'taki temel ve kullanıcı tanımlı türleri temsil eden basit bir yapı
// Gerçek bir dil için bu yapı çok daha detaylı ve karmaşık olacaktır (örneğin, yapısal türler, işaretçiler, referanslar vb.)
struct DppType {
    TypeKind kind;
    string name; // Temel türler için ismi (örneğin "int", "bool"), kullanıcı tanımlı türler için ismi
    // Kullanıcı tanımlı türler için referans (örneğin, bir StructDefinitionNode'a pointer)
     ASTNode* typeDefinition;

    // Diziler veya diğer bileşik türler için ek bilgiler olabilir.
     DppType* elementType; // Dizi eleman tipi

    bool isEquivalent(DppType other) const {
        // İki türün eşdeğer olup olmadığını kontrol eden basit bir mantık
        // Gerçekte, tip eşdeğerliği kuralları dile göre değişir (isimsel mi, yapısal mı vb.)
        return this.kind == other.kind && this.name == other.name;
    }
}

// Desteklenen türlerin türlerini belirten enum
enum TypeKind {
    Unknown,        // Bilinmeyen veya hata türü
    Void,           // Değer döndürmeyen fonksiyonlar için
    Int,            // Tam sayı
    Float,          // Kayan noktalı sayı
    Bool,           // Boolean
    String,         // Metin dizisi
    Struct,         // Kullanıcı tanımlı struct
    Class,          // Kullanıcı tanımlı sınıf
    Enum,           // Kullanıcı tanımlı enum
    // ... Diğer temel ve bileşik türler
}

// TypeKind enum değerini stringe çevirmek için yardımcı fonksiyon
string to!string(TypeKind kind) {
    final switch (kind) {
        case TypeKind.Unknown: return "Unknown";
        case TypeKind.Void: return "Void";
        case TypeKind.Int: return "int";
        case TypeKind.Float: return "float";
        case TypeKind.Bool: return "bool";
        case TypeKind.String: return "string";
        case TypeKind.Struct: return "struct";
        case TypeKind.Class: return "class";
        case TypeKind.Enum: return "enum";
        default: return format("TypeKind(%s)", kind);
    }
}

// Ön tanımlı temel türlerin örnekleri
immutable DppType IntType = {kind: TypeKind.Int, name: "int"};
immutable DppType FloatType = {kind: TypeKind.Float, name: "float"};
immutable DppType BoolType = {kind: TypeKind.Bool, name: "bool"};
immutable DppType StringType = {kind: TypeKind.String, name: "string"};
immutable DppType VoidType = {kind: TypeKind.Void, name: "void"};