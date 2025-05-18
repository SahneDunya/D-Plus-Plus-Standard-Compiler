#ifndef DPP_COMPILER_SEMA_TYPESYSTEM_H
#define DPP_COMPILER_SEMA_TYPESYSTEM_H

#include "../lexer/Token.h" // SourceLocation için (isteğe bağlı, Type içinde saklanabilir)
#include <string>
#include <vector>
#include <unordered_map>
#include <memory> // std::unique_ptr için
#include <functional> // std::hash için
#include <sstream> // stringstream için (tip isimleri oluştururken)

// İleri bildirimler
namespace dppc::sema { class Type; } // Type base sınıfı

// struct/class alanları, enum varyantları, trait metodları gibi bilgiler için yardımcı yapılar
namespace dppc {
namespace sema {

// Yapı Alanı Tanımı (StructField)
struct StructField {
    std::string Name; // Alan adı
    Type* type;       // Alanın tipi (TypeSystem'dan bir pointer)

    StructField(std::string name, Type* fieldType)
        : Name(std::move(name)), type(fieldType) {}
};

// Enum Varyant Tanımı (EnumVariant)
struct EnumVariant {
    std::string Name; // Varyant adı
    // TODO: İlişkili tipler (Rust gibi) veya sabit değer (D gibi) eklenebilir
     std::vector<Type*> AssociatedTypes; // Tuple variant için
     std::vector<StructField> AssociatedFields; // Struct variant için
     long long Value; // D'deki sabit değer

    EnumVariant(std::string name) : Name(std::move(name)) {}
};

// Trait Metot Tanımı (TraitMethod)
struct TraitMethod {
    std::string Name; // Metot adı
    Type* type;       // Metodun tipi (Fonksiyon tipi: FunctionType*)

    TraitMethod(std::string name, Type* methodType)
        : Name(std::move(name)), type(methodType) {}
};

} // namespace sema
} // namespace dppc


namespace dppc {
namespace sema {

// Tip Türleri (TypeKind)
enum class TypeKind {
    Error,          // Hata tipi (analiz edilemeyen tipler için)
    Void,           // void tip
    Boolean,        // bool tip
    Integer,        // Tamsayı tip (int, byte, short, long, uint, ubyte...)
    FloatingPoint,  // Kayan noktalı tip (float, double, real...)
    String,         // string tip
    Character,      // char tip
    Pointer,        // Pointer tip (*T)
    Reference,      // Referans tip (&T, &mut T)
    Array,          // Dizi tip ([T; N] veya [T])
    Slice,          // Slice tip ([T])
    Tuple,          // Tuple tip ((T1, T2, ...))
    Function,       // Fonksiyon/Delegate tip ((Args) -> ReturnType)
    Struct,         // Yapı (struct) tip
    Enum,           // Enum tip
    Trait,          // Trait tip (D++'a özgü)
    TraitObject,    // Trait Object tip (dyn Trait)
    Option,         // Option<T> tip (D++'a özgü)
    Result,         // Result<T, E> tip (D++'a özgü)
    // TODO: Class, Interface, Alias, Null, AA (Associative Array), Delegate, etc.
    // TODO: GenericType (Uninstantiated), GenericInstanceType (Instantiated Generic)
};

// Base Tip Sınıfı
class Type {
protected:
    TypeKind Kind; // Tipin türü

    // Constructor protected, sadece TypeSystem tarafından türetilmiş sınıflar oluşturulabilir
    Type(TypeKind kind) : Kind(kind) {}

public:
    // Sanal yıkıcı, kalıtım için gerekli
    virtual ~Type() = default;

    // Tip türünü döndürür
    TypeKind getKind() const { return Kind; }

    // Tipin string temsilini döndürür (örn: "int", "*float", "&mut MyStruct")
    virtual std::string getName() const = 0;

    // İki tipin anlamsal olarak eşit olup olmadığını kontrol eder
    virtual bool isEqual(const Type* other) const = 0;

    // Diğer Type* pointer'ını alan wrapper metodlar
    bool isEqual(const std::unique_ptr<Type>& other) const { return isEqual(other.get()); }
     bool isEqual(const Type& other) const { return isEqual(&other); }


    // Belirtilen tipe atanabilir olup olmadığını kontrol eder (Dest = Src)
    // Bu kontrol, implicit dönüşümleri, subtyping'i ve sahiplik/borrowing kurallarını içerebilir.
    // Atanabilirlik kuralları SemanticAnalyzer tarafından TypeSystem fonksiyonları çağrılarak kontrol edilir.
    // Bu sanal metot, tipin kendi özel atanabilirlik kurallarını tanımlayabilir.
    virtual bool isAssignableTo(const Type* destType) const;

     // Belirtilen tipe açıkça dönüştürülebilir (cast edilebilir) olup olmadığını kontrol eder
     virtual bool isConvertibleTo(const Type* destType) const;


    // Tipin belirli özelliklere sahip olup olmadığını kontrol eden helper metodlar
    bool isErrorType() const { return Kind == TypeKind::Error; }
    bool isVoidType() const { return Kind == TypeKind::Void; }
    bool isBooleanType() const { return Kind == TypeKind::Boolean; }
    bool isIntegerType() const { return Kind == TypeKind::Integer; }
    bool isFloatingPointType() const { return Kind == TypeKind::FloatingPoint; }
    bool isNumericType() const { return isIntegerType() || isFloatingPointType(); } // Sayısal tip mi?
    bool isStringType() const { return Kind == TypeKind::String; }
    bool isCharacterType() const { return Kind == TypeKind::Character; }
    bool isPointerType() const { return Kind == TypeKind::Pointer; }
    bool isReferenceType() const { return Kind == TypeKind::Reference; }
    bool isArrayType() const { return Kind == TypeKind::Array; }
    bool isSliceType() const { return Kind == TypeKind::Slice; }
    bool isTupleType() const { return Kind == TypeKind::Tuple; }
    bool isFunctionType() const { return Kind == TypeKind::Function; }
    bool isStructType() const { return Kind == TypeKind::Struct; }
    bool isEnumType() const { return Kind == TypeKind::Enum; }
    bool isTraitType() const { return Kind == TypeKind::Trait; }
    bool isTraitObjectType() const { return Kind == TypeKind::TraitObject; }
    bool isOptionType() const { return Kind == TypeKind::Option; }
    bool isResultType() const { return Kind == TypeKind::Result; }
    // TODO: isClassType, isInterfaceType, isUnionType, isNullable, isCopyType (D++ sahiplik için)

    // Tipin karşılaştırılabilir olup olmadığını kontrol eder (==, !=, <, > vb. için)
    // Genellikle aynı veya uyumlu tipler karşılaştırılabilir.
    virtual bool isComparableWith(const Type* other) const;

    // D++ Sahiplik ile ilgili helper'lar (örnek)
    // Tipin varsayılan olarak kopyalanabilir mi (move yerine copy)
    // struct'lar ve array'ler alanlarına/elementlerine bağlıdır.
    // İlkel tipler genellikle kopyalanabilir.
    virtual bool isCopyType() const;
};

// --- Türetilmiş Tip Sınıfları ---

// İlkel Tipler (int, float, bool, void, string, char)
class PrimitiveType : public Type {
private:
    // İlkel tipin spesifik alt türü (eğer farklı integer boyutları varsa)
    // enum class PrimitiveKind { Int, Float, Bool, Void, String, Char, UByte, SByte, Short, UShort, Long, ULong, Float32, Float64, Real, etc. };
    // PrimitiveKind SpecificKind; // Eğer farklı ilkel tipleri ayırt ediyorsanız

    std::string Name; // "int", "float", "bool", "void", "string", "char"

public:
    PrimitiveType(TypeKind kind, std::string name) : Type(kind), Name(std::move(name)) {}

    std::string getName() const override { return Name; }
    bool isEqual(const Type* other) const override;
    // isAssignableTo, isConvertibleTo, isComparableWith için varsayılan veya tip-spesifik implementasyonlar
    bool isNumericType() const override { return Kind == TypeKind::Integer || Kind == TypeKind::FloatingPoint; } // override edilebilir
    bool isBooleanType() const override { return Kind == TypeKind::Boolean; }
    bool isVoidType() const override { return Kind == TypeKind::Void; }
    bool isStringType() const override { return Kind == TypeKind::String; }
    bool isCharacterType() const override { return Kind == TypeKind::Character; }
    bool isCopyType() const override { return true; } // İlkel tipler genellikle kopyalanabilir
};

// Pointer Tip (*T)
class PointerType : public Type {
private:
    Type* BaseType; // İşaret edilen tip (TypeSystem'dan bir pointer)

public:
    PointerType(Type* baseType) : Type(TypeKind::Pointer), BaseType(baseType) {}

    Type* getBaseType() const { return BaseType; }
    std::string getName() const override;
    bool isEqual(const Type* other) const override;
    // isAssignableTo, isConvertibleTo, isComparableWith için implementasyonlar
};

// Referans Tip (&T, &mut T)
class ReferenceType : public Type {
private:
    Type* BaseType; // İşaret edilen tip
    bool IsMutable; // mutable referans mı?

public:
    ReferenceType(Type* baseType, bool isMutable) : Type(TypeKind::Reference), BaseType(baseType), IsMutable(isMutable) {}

    Type* getBaseType() const { return BaseType; }
    bool isMutable() const { return IsMutable; }
    std::string getName() const override;
    bool isEqual(const Type* other) const override;
    // isAssignableTo (örn: &mut T -> &T), isConvertibleTo, isComparableWith için implementasyonlar
     bool isCopyType() const override { return true; } // Referanslar genellikle kopyalanabilir (pointer gibi)
};

// Dizi Tip ([T; N] veya [T])
class ArrayType : public Type {
private:
    Type* ElementType; // Dizi elementi tipi
    long long Size;    // Dizi boyutu (-1 ise boyutu bilinmeyen dizi/slice)

public:
    ArrayType(Type* elementType, long long size) : Type(TypeKind::Array), ElementType(elementType), Size(size) {}
     // slice için constructor (boyut -1)
    ArrayType(Type* elementType) : Type(TypeKind::Slice), ElementType(elementType), Size(-1) {}


    Type* getElementType() const { return ElementType; }
    long long getSize() const { return Size; } // Eğer Kind Slice ise -1 döner

    std::string getName() const override;
    bool isEqual(const Type* other) const override;
    // isAssignableTo, isConvertibleTo, isComparableWith için implementasyonlar
    // isCopyType, element tipine bağlıdır
};

// Tuple Tip ((T1, T2, ...))
class TupleType : public Type {
private:
    std::vector<Type*> ElementTypes; // Tuple element tipleri

public:
    TupleType(std::vector<Type*> elementTypes) : Type(TypeKind::Tuple), ElementTypes(std::move(elementTypes)) {}

    const std::vector<Type*>& getElementTypes() const { return ElementTypes; }

    std::string getName() const override;
    bool isEqual(const Type* other) const override;
    // isAssignableTo, isConvertibleTo, isComparableWith için implementasyonlar
    // isCopyType, element tiplerine bağlıdır
};

// Fonksiyon Tip ((Args) -> ReturnType)
class FunctionType : public Type {
private:
    Type* ReturnType;             // Dönüş tipi
    std::vector<Type*> ParameterTypes; // Parametre tipleri

public:
    FunctionType(Type* returnType, std::vector<Type*> paramTypes)
        : Type(TypeKind::Function), ReturnType(returnType), ParameterTypes(std::move(paramTypes)) {}

    Type* getReturnType() const { return ReturnType; }
    const std::vector<Type*>& getParameterTypes() const { return ParameterTypes; }

    std::string getName() const override;
    bool isEqual(const Type* other) const override;
    // isAssignableTo (fonksiyon pointer'ları için), isConvertibleTo, isComparableWith için implementasyonlar
};


// Yapı (Struct) Tip
class StructType : public Type {
private:
    std::string Name; // Yapı adı
    std::vector<StructField> Fields; // Yapı alanları
    // TODO: Metotlar, ilişkili tipler, jenerik parametreler eklenebilir

public:
    StructType(std::string name) : Type(TypeKind::Struct), Name(std::move(name)) {}

    const std::string& getName() const override { return Name; }
    bool isEqual(const Type* other) const override; // Yapı adlarına göre eşitlik (veya pointer eşitliği)
    const std::vector<StructField>& getFields() const { return Fields; }
    const StructField* getField(const std::string& name) const; // Alanı adına göre bul
    void setFields(std::vector<StructField> fields) { Fields = std::move(fields); } // Alanları ayarla (Sema'dan çağrılır)

    // isAssignableTo, isConvertibleTo için implementasyonlar (alanlara göre karşılaştırma/dönüşüm)
    // isCopyType, alan tiplerine bağlıdır
};

// Enum Tip
class EnumType : public Type {
private:
    std::string Name; // Enum adı
    std::vector<EnumVariant> Variants; // Enum varyantları

public:
    EnumType(std::string name) : Type(TypeKind::Enum), Name(std::move(name)) {}

    const std::string& getName() const override { return Name; }
    bool isEqual(const Type* other) const override; // Enum adlarına göre eşitlik
    const std::vector<EnumVariant>& getVariants() const { return Variants; }
    const EnumVariant* getVariant(const std::string& name) const; // Varyantı adına göre bul
    void setVariants(std::vector<EnumVariant> variants) { Variants = std::move(variants); } // Varyantları ayarla (Sema'dan çağrılır)

    // isAssignableTo, isConvertibleTo için implementasyonlar
    // isCopyType genellikle true'dur (enum değerleri genellikle kopyalanabilir)
};


// Trait Tip (D++'a özgü)
class TraitType : public Type {
private:
    std::string Name; // Trait adı
    std::vector<TraitMethod> RequiredMethods; // Gerektirilen metodlar
    // TODO: Gerektirilen ilişkili tipler, sabitler eklenebilir

public:
    TraitType(std::string name) : Type(TypeKind::Trait), Name(std::move(name)) {}

    const std::string& getName() const override { return Name; }
    bool isEqual(const Type* other) const override; // Trait adlarına göre eşitlik
    const std::vector<TraitMethod>& getRequiredMethods() const { return RequiredMethods; }
    const TraitMethod* getRequiredMethod(const std::string& name) const; // Gerektirilen metodu adına göre bul
    void setRequiredMethods(std::vector<TraitMethod> methods) { RequiredMethods = std::move(methods); } // Metotları ayarla (Sema'dan çağrılır)

    // Trait tipi genellikle kendisi atanabilir/dönüştürülebilir değildir, TraitObject kullanılır.
};


// Option<T> Tip (D++'a özgü)
class OptionType : public Type {
private:
    Type* InnerType; // T tipi

public:
    OptionType(Type* innerType) : Type(TypeKind::Option), InnerType(innerType) {}

    Type* getInnerType() const { return InnerType; }
    std::string getName() const override;
    bool isEqual(const Type* other) const override; // İçteki tipe göre eşitlik
    // isAssignableTo, isConvertibleTo için implementasyonlar
    // isCopyType, içteki tipe bağlıdır
};

// Result<T, E> Tip (D++'a özgü)
class ResultType : public Type {
private:
    Type* OkType;  // Başarılı durumdaki değerin tipi (T)
    Type* ErrType; // Hata durumundaki değerin tipi (E)

public:
    ResultType(Type* okType, Type* errType) : Type(TypeKind::Result), OkType(okType), ErrType(errType) {}

    Type* getOkType() const { return OkType; }
    Type* getErrType() const { return ErrType; }
    std::string getName() const override;
    bool isEqual(const Type* other) const override; // İçteki tiplere göre eşitlik
    // isAssignableTo, isConvertibleTo için implementasyonlar
    // isCopyType, içteki tiplere bağlıdır
};


// --- TypeSystem Sınıfı ---

// Tuple, Array, Function, Pointer, Reference tiplerini unique yapmak için hash ve eşitlik yardımcıları
// std::unordered_map kullanmak için bu tiplerin hashlenebilir olması gerekir.

// Type* vektörünü hashlemek için yardımcı yapı (Tuple ve Function tipleri için)
struct TypeVectorHash {
    size_t operator()(const std::vector<Type*>& vec) const {
        std::hash<const Type*> hasher;
        size_t seed = 0;
        for (const Type* type : vec) {
            // Basit bir hash birleştirme tekniği
            seed ^= hasher(type) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

// Type* vektörünü karşılaştırmak için yardımcı yapı (Tuple ve Function tipleri için)
struct TypeVectorEqual {
    bool operator()(const std::vector<Type*>& lhs, const std::vector<Type*>& rhs) const {
        if (lhs.size() != rhs.size()) return false;
        for (size_t i = 0; i < lhs.size(); ++i) {
            // Type pointer'larını karşılaştır (aynı Type nesnesi mi?)
            // veya Type::isEqual metodunu kullan: lhs[i]->isEqual(rhs[i])
            if (lhs[i] != rhs[i]) return false; // Basit pointer karşılaştırması
        }
        return true;
    }
};

// Type* pointer'ını hashlemek için yardımcı yapı (Pointer, Reference için)
struct TypePointerHash {
     size_t operator()(const Type* type) const {
         return std::hash<const Type*>()(type);
     }
};

// Type* pointer'ını karşılaştırmak için yardımcı yapı (Pointer, Reference için)
struct TypePointerEqual {
     bool operator()(const Type* lhs, const Type* rhs) const {
         return lhs == rhs; // Pointer eşitliği
     }
};


class TypeSystem {
private:
    // Tüm oluşturulan Type nesnelerinin sahipliği bu vektörde tutulur
    std::vector<std::unique_ptr<Type>> AllTypes;

    // Tekil (Singleton) İlkel Tiplerin Pointer'ları
    Type* ErrorTy = nullptr;
    Type* VoidTy = nullptr;
    Type* BooleanTy = nullptr;
    Type* IntTy = nullptr;
    Type* FloatTy = nullptr; // float, double, real için tek veya ayrı ayrı
    Type* StringTy = nullptr;
    Type* CharTy = nullptr;
    // TODO: Diğer ilkel tipler için pointer'lar (ubyte, short, ulong vb.)

    // Uniquleştirme için Map'ler
    // Named Types (Struct, Enum, Trait) isimlerine göre bulunur
    std::unordered_map<std::string, Type*> NamedTypes;
    // Pointer Tipler temel tipine göre bulunur
    std::unordered_map<Type*, Type*, TypePointerHash, TypePointerEqual> PointerTypes;
    // Referans Tipler temel tip ve mutability'ye göre bulunur
    std::unordered_map<std::pair<Type*, bool>, Type*, std::hash<Type*>, std::equal_to<Type*>> ReferenceTypes; // std::pair hash/equal'a ihtiyaç duyar veya custom
    // Array/Slice Tipler element tipi ve boyutuna göre bulunur
    std::unordered_map<std::pair<Type*, long long>, Type*, std::hash<Type*>, std::equal_to<Type*>> ArrayTypes; // std::pair hash/equal'a ihtiyaç duyar veya custom
    // Tuple Tipler element tipleri vektörüne göre bulunur
    std::unordered_map<std::vector<Type*>, Type*, TypeVectorHash, TypeVectorEqual> TupleTypes;
     // Function Tipler dönüş tipi ve parametre tipleri vektörüne göre bulunur
     std::unordered_map<std::pair<Type*, std::vector<Type*>>, Type*, std::hash<Type*>, TypeVectorEqual> FunctionTypes; // Pair hash/equal'a ihtiyaç duyar

    // D++ Özgü Tipler
    std::unordered_map<Type*, Type*, TypePointerHash, TypePointerEqual> OptionTypes; // İçteki tipe göre
     std::unordered_map<std::pair<Type*, Type*>, Type*, std::hash<Type*>, std::equal_to<Type*>> ResultTypes; // Ok ve Err tiplerine göre


    // Yeni bir Type nesnesi oluşturur ve AllTypes vektörüne ekleyip pointer'ını döndürür
    template<typename T, typename... Args>
    T* createType(Args&&... args) {
        auto type = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = type.get();
        AllTypes.push_back(std::move(type));
        return ptr;
    }


public:
    // Constructor
    TypeSystem();

    // Yıkıcı: Tüm Type nesnelerini temizler (AllTypes vektörü unique_ptr'lar sayesinde)
    ~TypeSystem() = default;

    // --- Tip Fabrika Metotları (Uniquleştirme sağlar) ---

    Type* getErrorType() const { return ErrorTy; }
    Type* getVoidType() const { return VoidTy; }
    Type* getBooleanType() const { return BooleanTy; }
    Type* getIntType() const { return IntTy; }
    Type* getFloatType() const { return FloatTy; } // float, double, real için tek veya ayrı
    Type* getStringType() const { return StringTy; }
    Type* getCharacterType() const { return CharTy; }
    // TODO: Diğer ilkel tipler için get metodları (getUbyteType, getShortType vb.)

    // İşaret edilen temel tipe göre PointerType oluştur/getir
    Type* getPointerType(Type* baseType);

    // İşaret edilen temel tip ve mutability'ye göre ReferenceType oluştur/getir
    Type* getReferenceType(Type* baseType, bool isMutable);

    // Element tipi ve opsiyonel boyuta göre ArrayType veya SliceType oluştur/getir
    Type* getArrayType(Type* elementType, long long size);
    Type* getSliceType(Type* elementType); // ArrayType(elementType, -1) ile aynı

    // Element tipleri vektörüne göre TupleType oluştur/getir
    Type* getTupleType(const std::vector<Type*>& elementTypes);

    // Dönüş tipi ve parametre tipleri vektörüne göre FunctionType oluştur/getir
    Type* getFunctionType(Type* returnType, const std::vector<Type*>& parameterTypes);


    // Named Types (Struct, Enum, Trait) - Bu tipler genellikle Semantik Analiz sırasında tanımlanır
    // Kayıt metodları Semantik Analizci tarafından kullanılır.
    // Yeni bir StructType bildirimi geldiğinde çağrılır (alanlar daha sonra set edilir)
    StructType* createStructType(const std::string& name);
    // Mevcut StructType'ın alanlarını set etmek için (Sema analiz ettikten sonra çağırır)
    void updateStructType(StructType* structType, std::vector<StructField> fields);

    // Yeni bir EnumType bildirimi geldiğinde çağrılır (varyantlar daha sonra set edilir)
    EnumType* createEnumType(const std::string& name);
    // Mevcut EnumType'ın varyantlarını set etmek için (Sema analiz ettikten sonra çağırır)
    void updateEnumType(EnumType* enumType, std::vector<EnumVariant> variants);

    // Yeni bir TraitType bildirimi geldiğinde çağrılır (metodlar daha sonra set edilir)
    TraitType* createTraitType(const std::string& name);
    // Mevcut TraitType'ın gerektirdiği metodları set etmek için (Sema analiz ettikten sonra çağırır)
    void updateTraitType(TraitType* traitType, std::vector<TraitMethod> methods);

    // Named type'ı adına göre bul (Sembol Tablosu lookup sonrası Type* elde etmek için kullanılır)
    Type* getNamedType(const std::string& name) const;


    // D++ Özgü Tip Fabrika Metotları

    // İçteki T tipine göre Option<T> oluştur/getir
    Type* getOptionType(Type* innerType); // Eğer innerType nullptr ise, jenerik Option tipini temsil edebilir (kullanımına bağlı)

    // Ok ve Err tiplerine göre Result<T, E> oluştur/getir
    Type* getResultType(Type* okType, Type* errType); // Eğer tipler nullptr ise, jenerik Result tipini temsil edebilir


    // --- Tip Karşılaştırma ve Sorgulama Metotları ---
    // Bu metodlar genellikle Type* pointer'larını alır ve Type sınıfının sanal metodlarını çağırır.

    bool isEqual(const Type* type1, const Type* type2) const;
    bool isAssignable(const Type* destType, const Type* srcType) const; // Dest = Src geçerli mi?
    bool isConvertible(const Type* fromType, const Type* toType) const; // Cast geçerli mi?
    bool isComparable(const Type* type1, const Type* type2) const; // ==, !=, <, > vb. için geçerli mi?

    // İki sayısal tipin ortak geniş tipini bul (örn: int ve float -> float)
    // Bu metod TypeSystem'ın sayısal tip hiyerarşisi bilgisine sahip olmasını gerektirir.
    Type* getWiderNumericType(const Type* type1, const Type* type2) const;

    // Type* pointer'ının geçerli bir tip nesnesini gösterip göstermediğini kontrol et (ErrorType değil mi?)
    bool isValidType(const Type* type) const { return type != nullptr && type != ErrorTy; }

     // Debug için tüm oluşturulmuş tipleri listele
     void dumpCreatedTypes() const;
};

} // namespace sema
} // namespace dppc

// std::pair<Type*, bool> için hash ve eşitlik yapılarını std namespace'inde tanımlamak,
// std::unordered_map ile kullanabilmek için gereklidir.
// Veya custom bir key struct tanımlayıp onun hash ve eşitliğini implement edebilirsiniz.
// Basitlik için std::pair'in default hash/equal'ını kullanmayı deneyelim, Type* pointer'larının hash/equal'ı yeterli olabilir.

// std::pair<Type*, Type*> için hash
namespace std {
    template<> struct hash<pair<dppc::sema::Type*, dppc::sema::Type*>> {
        size_t operator()(const pair<dppc::sema::Type*, dppc::sema::Type*>& p) const {
            auto h1 = hash<dppc::sema::Type*>()(p.first);
            auto h2 = hash<dppc::sema::Type*>()(p.second);
            return h1 ^ (h2 << 1); // Basit bir birleştirme
        }
    };

    // std::pair<Type*, bool> için hash
     template<> struct hash<pair<dppc::sema::Type*, bool>> {
         size_t operator()(const pair<dppc::sema::Type*, bool>& p) const {
             auto h1 = hash<dppc::sema::Type*>()(p.first);
             auto h2 = hash<bool>()(p.second);
             return h1 ^ (h2 << 1); // Basit bir birleştirme
         }
     };
}


#endif // DPP_COMPILER_SEMA_TYPESYSTEM_H
