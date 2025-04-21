module types;

import std.string;
import std.array;
import syntax_kind; // İlgili SyntaxKind'lar için (TypeKind yerine daha detaylı olabilir)
import ast;         // Kullanıcı tanımlı türlerin AST düğümüne referans için (isteğe bağlı)

// Farklı türlerin temel kategorilerini belirten enum
// Bu, syntax_kind.d'deki genel AST/Token türlerinden daha spesifiktir.
enum DppTypeKind {
    Unknown,          // Bilinmeyen veya hata türü
    Void,             // Değer döndürmeyen
    Primitive,        // int, float, bool, char gibi temel türler
    String,           // String türü (genellikle primitive'den ayrı ele alınır)
    Struct,           // Kullanıcı tanımlı struct
    Class,            // Kullanıcı tanımlı sınıf
    Enum,             // Kullanıcı tanımlı enum
    Pointer,          // İşaretçi türü (*T)
    Reference,        // Referans türü (&T, &mut T - Rust esintili)
    Array,            // Dizi türü ([T; size] veya []T)
    Slice,            // Dilim türü ([]T)
    Tuple,            // Tuple türü ((T1, T2))
    Function,         // Fonksiyon türü (fn(Args...) -> ReturnType)
    // ... Diğer tür kategorileri (TraitObject, Closure, vb.)
}


// Tüm D++ türlerini temsil eden temel sınıf (Polimorfizm için)
abstract class DppType {
    DppTypeKind kind;
    string name; // Türün okunabilir ismi (debugging için)

    this(DppTypeKind kind, string name) {
        this.kind = kind;
        this.name = name;
    }

    // İki türün tam olarak aynı olup olmadığını kontrol eder (isimsel veya yapısal eşitlik kurallarına göre)
    abstract bool isExactly(const DppType other) const;

    // Bu türün belirtilen türe atanabilir olup olmadığını kontrol eder (tür yükseltme vb. kurallarla)
    // Bu fonksiyon type_system.d'de de bulunabilir ve burada çağrılabilir.
     bool isAssignableTo(const DppType targetType) const;

    // Türü stringe çevirmek için yardımcı fonksiyon
    abstract string toString() const;
}

// Temel (Primitive) türleri temsil eden sınıf
class PrimitiveType : DppType {
    // Özel bir bilgiye gerek yok, kind ve name yeterli.
    this(DppTypeKind kind, string name) {
        super(kind, name);
        assert(kind == DppTypeKind.Primitive || kind == DppTypeKind.Void || kind == DppTypeKind.String); // Sadece primitive kategorisindekiler
    }

    override bool isExactly(const DppType other) const {
        // Primitive türler için kind ve name eşitliği yeterlidir.
        // String ve Void de burada ele alınabilir veya ayrı PrimitiveCategory enum'ı eklenebilir.
        if (auto otherPrimitive = cast(const(PrimitiveType))other) {
            return this.kind == otherPrimitive.kind && this.name == otherPrimitive.name;
        }
        return false;
    }

    override string toString() const {
        return name;
    }
}

// Kullanıcı tanımlı struct türlerini temsil eden sınıf
class StructType : DppType {
    string structName; // Struct'ın asıl adı
    // ASTNode* definitionNode; // Struct tanımının AST düğümüne referans (isteğe bağlı)
    // SymbolTable* membersScope; // Struct üyelerinin (alanlar, metotlar) sembol tablosu (isteğe bağlı ama kullanışlı)
    StructMember[] members; // Struct'ın alanları

    this(string structName, StructMember[] members) {
        super(DppTypeKind.Struct, structName);
        this.structName = structName;
        this.members = members;
    }

    override bool isExactly(const DppType other) const {
        // Struct'lar için isimsel eşitlik (aynı isimdeki struct aynı struct'tır)
        // veya yapısal eşitlik (alanları aynıysa aynıdır) kurallarına bağlıdır.
        // Basitlik için isimsel eşitlik varsayalım.
        if (auto otherStruct = cast(const(StructType))other) {
            return this.structName == otherStruct.structName;
        }
        return false;
    }

    override string toString() const {
        return "struct " ~ structName;
    }
}

// Struct üyelerini (alanları) temsil eden yapı
struct StructMember {
    string name;   // Alanın adı
    DppType* type; // Alanın türü
    // int offset; // Bellek offseti (backend için)
}

// İşaretçi türlerini temsil eden sınıf (Örnek: *int, *MyStruct)
class PointerType : DppType {
    DppType* baseType; // İşaret edilen tür

    this(DppType* baseType) {
        super(DppTypeKind.Pointer, baseType.toString() ~ "*");
        this.baseType = baseType;
    }

    override bool isExactly(const DppType other) const {
        if (auto otherPointer = cast(const(PointerType))other) {
            // Temel tipleri tam olarak aynıysa işaretçi tipleri de aynıdır.
            return this.baseType.isExactly(otherPointer.baseType);
        }
        return false;
    }

    override string toString() const {
        return baseType.toString() ~ "*";
    }
}

// Referans türlerini temsil eden sınıf (Örnek: &int, &mut MyStruct) - Rust esintili
class ReferenceType : DppType {
    DppType* baseType; // Referans verilen tür
    bool isMutable;    // Mutable referans mı?

    this(DppType* baseType, bool isMutable) {
        super(DppTypeKind.Reference, (isMutable ? "&mut " : "&") ~ baseType.toString());
        this.baseType = baseType;
        this.isMutable = isMutable;
    }

    override bool isExactly(const DppType other) const {
         if (auto otherReference = cast(const(ReferenceType))other) {
             // Temel tipleri tam olarak aynıysa ve değiştirilebilirlik durumları aynıysa referans tipleri de aynıdır.
             return this.baseType.isExactly(otherReference.baseType) && this.isMutable == otherReference.isMutable;
         }
         return false;
    }

     override string toString() const {
        return (isMutable ? "&mut " : "&") ~ baseType.toString();
    }
}

// Fonksiyon türlerini temsil eden sınıf (Örnek: fn(int, string) -> bool)
class FunctionType : DppType {
    DppType* returnType; // Dönüş türü
    DppType*[] parameterTypes; // Parametre türleri listesi

    this(DppType* returnType, DppType*[] parameterTypes) {
        super(DppTypeKind.Function, "fn(...) -> ..."); // Debugging ismi daha detaylı olabilir
        this.returnType = returnType;
        this.parameterTypes = parameterTypes;
        this.name = toString(); // İsmi string fonksiyonu ile oluştur
    }

    override bool isExactly(const DppType other) const {
        if (auto otherFunction = cast(const(FunctionType))other) {
            // Dönüş tipleri aynı olmalı ve parametre tipleri listeleri tam olarak aynı olmalı.
            if (!this.returnType.isExactly(otherFunction.returnType) || this.parameterTypes.length != otherFunction.parameterTypes.length) {
                return false;
            }
            for (size_t i = 0; i < this.parameterTypes.length; ++i) {
                if (!this.parameterTypes[i].isExactly(otherFunction.parameterTypes[i])) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    override string toString() const {
        string paramStr = "(";
        foreach (i, paramType; parameterTypes) {
            paramStr ~= paramType.toString();
            if (i < parameterTypes.length - 1) {
                paramStr ~= ", ";
            }
        }
        paramStr ~= ")";
        return "fn" ~ paramStr ~ " -> " ~ returnType.toString();
    }
}


// Ön tanımlı temel türlerin instance'ları
// Bu instance'lar program boyunca aynı olmalıdır (singleton gibi)
immutable PrimitiveType IntType = new PrimitiveType(DppTypeKind.Primitive, "int");
immutable PrimitiveType FloatType = new PrimitiveType(DppTypeKind.Primitive, "float");
immutable PrimitiveType BoolType = new PrimitiveType(DppTypeKind.Primitive, "bool");
immutable PrimitiveType StringType = new PrimitiveType(DppTypeKind.String, "string"); // String ayrı bir kategori olabilir
immutable PrimitiveType VoidType = new PrimitiveType(DppTypeKind.Void, "void");


// Bir SyntaxKind'dan temel DppType instance'ını almak için yardımcı fonksiyon
// Bu, lexer/parser tarafından üretilen literal tokenlardan veya tip belirtimi AST düğümlerinden
// DppType nesnelerine geçiş yaparken kullanılabilir.
DppType* getBasicTypeFromSyntaxKind(SyntaxKind kind) {
    final switch (kind) {
        case SyntaxKind.IntegerLiteralToken: return IntType;
        case SyntaxKind.FloatingPointLiteralToken: return FloatType;
        case SyntaxKind.StringLiteralToken: return StringType;
        case SyntaxKind.BooleanLiteralToken: return BoolType;
        case SyntaxKind.IntKeyword: return IntType; // Tip belirtimi keyword'leri
        case SyntaxKind.FloatKeyword: return FloatType;
        // ... diğer temel tiplerin keyword'leri
        default: return null; // Bilinmeyen temel tip
    }
}

// DppType pointerları için çöp toplama (eğer sınıf kullanıyorsanız)
// Dikkat: Eğer DppType struct olsaydı ve manuel bellek yönetimi yapsaydınız,
// bu nesnelerin yaşam döngüsünü dikkatlice yönetmeniz gerekirdi.
// D'nin GC'si sınıflar için bunu otomatik halleder.