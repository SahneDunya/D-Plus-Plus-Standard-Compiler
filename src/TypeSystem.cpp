#include "TypeSystem.h"
#include <algorithm> // std::find_if için

namespace dppc {
namespace sema {

// --- Base Type Sınıfı Implementasyonu ---

// isAssignableTo için varsayılan implementasyon (sadece aynı tipe atanabilir)
bool Type::isAssignableTo(const Type* destType) const {
    // Hata tipleri atanabilir değil
    if (this->isErrorType() || (destType && destType->isErrorType())) return false;
    // Varsayılan olarak sadece aynı tipin atanabilir olduğunu varsayalım
    return isEqual(destType);
}

// isConvertibleTo için varsayılan implementasyon (sadece aynı tipe dönüştürülebilir)
bool Type::isConvertibleTo(const Type* destType) const {
    // Hata tipleri dönüştürülebilir değil
    if (this->isErrorType() || (destType && destType->isErrorType())) return false;
    // Varsayılan olarak sadece aynı tipe dönüştürülebilir olduğunu varsayalım
    return isEqual(destType);
}

// isComparableWith için varsayılan implementasyon (sadece aynı tipe karşılaştırılabilir)
bool Type::isComparableWith(const Type* other) const {
     // Hata tipleri karşılaştırılamaz
    if (this->isErrorType() || (other && other->isErrorType())) return false;
    // Varsayılan olarak sadece aynı tipin karşılaştırılabilir olduğunu varsayalım
    return isEqual(other);
}

// isCopyType için varsayılan implementasyon (çoğu ilkel tip kopyalanabilir)
bool Type::isCopyType() const {
    // İlkel tipler genellikle kopyalanabilir.
    // Bileşik tipler (Struct, Array, Tuple) içerdikleri tiplerin isCopyType() durumuna bağlıdır.
    // Pointerlar ve Referanslar genellikle kopyalanabilir (referansın kendisi, işaret ettiği değer değil).
    switch (Kind) {
        case TypeKind::Boolean:
        case TypeKind::Integer:
        case TypeKind::FloatingPoint:
        case TypeKind::String: // D'de string mutable değildir ve kopyalanabilir
        case TypeKind::Character:
        case TypeKind::Pointer:
        case TypeKind::Reference:
        case TypeKind::Function: // Fonksiyon pointerları/delegateler kopyalanabilir
        case TypeKind::Enum: // Enum değerleri kopyalanabilir
            return true;
        case TypeKind::Void:
        case TypeKind::Error:
        case TypeKind::Struct:
        case TypeKind::Array:
        case TypeKind::Slice:
        case TypeKind::Tuple:
        case TypeKind::Trait: // Trait tipi kendisi kopyalanamaz (TraitObject kopyalanabilirliği farklı)
        case TypeKind::TraitObject: // Trait object kopyalanabilirliği temsil ettiği tipe bağlı olabilir
        case TypeKind::Option:
        case TypeKind::Result:
             // Bu tipler için isCopyType implementasyonu kendi sınıflarında yapılmalıdır.
            return false; // Varsayılan olarak kopyalanamaz
        default:
            return false;
    }
}


// --- Türetilmiş Tip Sınıfları Implementasyonları ---

// PrimitiveType Implementasyonu
bool PrimitiveType::isEqual(const Type* other) const {
    if (this == other) return true; // Pointer eşitliği
    if (const PrimitiveType* otherPrim = dynamic_cast<const PrimitiveType*>(other)) {
        // Aynı ilkel tür mü? (Eğer PrimitiveKind kullanıyorsanız burayı kontrol edin)
        // veya sadece TypeKind yeterli mi? (örn: tüm integerlar aynı Kind altında ama farklı Name'e sahip)
        // Basitlik için aynı Kind ve aynı Name kontrolü yapalım:
        return this->Kind == otherPrim->Kind && this->Name == otherPrim->Name;
         return this->SpecificKind == otherPrim->SpecificKind; // Eğer PrimitiveKind kullanıyorsanız
    }
    return false;
}
// TODO: PrimitiveType için isAssignableTo, isConvertibleTo, isComparableWith implementasyonları

// PointerType Implementasyonu
std::string PointerType::getName() const {
    return "*" + BaseType->getName();
}
bool PointerType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const PointerType* otherPtr = dynamic_cast<const PointerType*>(other)) {
        // İşaret ettikleri tipler eşit mi?
        return BaseType->isEqual(otherPtr->BaseType);
    }
    return false;
}
// TODO: PointerType için isAssignableTo, isConvertibleTo, isComparableWith implementasyonları

// ReferenceType Implementasyonu
std::string ReferenceType::getName() const {
    return (IsMutable ? "&mut " : "&") + BaseType->getName();
}
bool ReferenceType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const ReferenceType* otherRef = dynamic_cast<const ReferenceType*>(other)) {
        // İşaret ettikleri tipler eşit mi ve mutability aynı mı?
        return BaseType->isEqual(otherRef->BaseType) && IsMutable == otherRef->IsMutable;
    }
    return false;
}
// TODO: ReferenceType için isAssignableTo (&mut T -> &T geçerli olabilir), isConvertibleTo, isComparableWith implementasyonları

// ArrayType Implementasyonu
std::string ArrayType::getName() const {
    std::stringstream ss;
    ss << "[" << ElementType->getName();
    if (Kind == TypeKind::Array) { // Sabit boyutlu dizi
        ss << "; " << Size;
    }
    ss << "]";
    return ss.str();
}
bool ArrayType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const ArrayType* otherArray = dynamic_cast<const ArrayType*>(other)) {
        // Aynı tür mü (Array veya Slice)? Element tipi eşit mi? Boyut eşit mi (sadece Array için)?
        return this->Kind == otherArray->Kind &&
               ElementType->isEqual(otherArray->ElementType) &&
               (this->Kind == TypeKind::Slice || this->Size == otherArray->Size);
    }
    return false;
}
// TODO: ArrayType için isAssignableTo ([T; N] -> [T], [T] -> [const T] gibi), isConvertibleTo, isComparableWith, isCopyType implementasyonları

// TupleType Implementasyonu
std::string TupleType::getName() const {
    std::stringstream ss;
    ss << "(";
    for (size_t i = 0; i < ElementTypes.size(); ++i) {
        ss << ElementTypes[i]->getName();
        if (i < ElementTypes.size() - 1) {
            ss << ", ";
        }
    }
    ss << ")";
    return ss.str();
}
bool TupleType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const TupleType* otherTuple = dynamic_cast<const TupleType*>(other)) {
        // Element sayıları ve karşılıklı element tipleri eşit mi?
        if (ElementTypes.size() != otherTuple->ElementTypes.size()) return false;
        for (size_t i = 0; i < ElementTypes.size(); ++i) {
            if (!ElementTypes[i]->isEqual(otherTuple->ElementTypes[i])) return false;
        }
        return true;
    }
    return false;
}
// TODO: TupleType için isAssignableTo, isConvertibleTo, isComparableWith, isCopyType implementasyonları

// FunctionType Implementasyonu
std::string FunctionType::getName() const {
    std::stringstream ss;
    ss << "(";
    for (size_t i = 0; i < ParameterTypes.size(); ++i) {
        ss << ParameterTypes[i]->getName();
        if (i < ParameterTypes.size() - 1) {
            ss << ", ";
        }
    }
    ss << ") -> " << ReturnType->getName();
    return ss.str();
}
bool FunctionType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const FunctionType* otherFunc = dynamic_cast<const FunctionType*>(other)) {
        // Dönüş tipleri eşit mi ve parametre tipleri eşit mi (sayı ve karşılıklı tipler)?
        if (!ReturnType->isEqual(otherFunc->ReturnType)) return false;
        if (ParameterTypes.size() != otherFunc->ParameterTypes.size()) return false;
        for (size_t i = 0; i < ParameterTypes.size(); ++i) {
            if (!ParameterTypes[i]->isEqual(otherFunc->ParameterTypes[i])) return false;
        }
        return true;
    }
    return false;
}
// TODO: FunctionType için isAssignableTo (Delegate uyumluluğu, function pointer uyumluluğu), isConvertibleTo, isComparableWith implementasyonları

// StructType Implementasyonu
const StructField* StructType::getField(const std::string& name) const {
     auto it = std::find_if(Fields.begin(), Fields.end(),
                             [&](const StructField& field){ return field.Name == name; });
     if (it != Fields.end()) {
         return &(*it); // Alan bulundu
     }
     return nullptr; // Alan bulunamadı
}
bool StructType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const StructType* otherStruct = dynamic_cast<const StructType*>(other)) {
        // Structlar genellikle adlarına göre eşit kabul edilir (Nominal Typing)
        // Aynı ada sahip iki struct farklı alanlara sahip olsa bile tipleri farklıdır.
        // Tam eşitlik için alanları da karşılaştırmak gerekebilir, ancak çoğu dilde aynı isimli struct aynı tiptir.
        return this->Name == otherStruct->Name;
    }
    return false;
}
// TODO: StructType için isAssignableTo, isConvertibleTo, isCopyType implementasyonları (alanlara göre)

// EnumType Implementasyonu
const EnumVariant* EnumType::getVariant(const std::string& name) const {
    auto it = std::find_if(Variants.begin(), Variants.end(),
                             [&](const EnumVariant& variant){ return variant.Name == name; });
     if (it != Variants.end()) {
         return &(*it); // Varyant bulundu
     }
     return nullptr; // Varyant bulunamadı
}
bool EnumType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const EnumType* otherEnum = dynamic_cast<const EnumType*>(other)) {
        // Enumlar genellikle adlarına göre eşit kabul edilir.
        return this->Name == otherEnum->Name;
    }
    return false;
}
// TODO: EnumType için isAssignableTo, isConvertibleTo, isCopyType implementasyonları

// TraitType Implementasyonu (D++'a özgü)
const TraitMethod* TraitType::getRequiredMethod(const std::string& name) const {
    auto it = std::find_if(RequiredMethods.begin(), RequiredMethods.end(),
                             [&](const TraitMethod& method){ return method.Name == name; });
     if (it != RequiredMethods.end()) {
         return &(*it); // Metod bulundu
     }
     return nullptr; // Metod bulunamadı
}
bool TraitType::isEqual(const Type* other) const {
     if (this == other) return true;
    if (const TraitType* otherTrait = dynamic_cast<const TraitType*>(other)) {
        // Traitler genellikle adlarına göre eşit kabul edilir.
        return this->Name == otherTrait->Name;
    }
    return false;
}
// TODO: TraitType için isAssignableTo, isConvertibleTo implementasyonları (Genellikle TraitObject üzerinden kullanılır)


// OptionType Implementasyonu (D++'a özgü)
std::string OptionType::getName() const {
    if (!InnerType) return "Option<?>"; // Jenerik Option tipi veya hata
    return "Option<" + InnerType->getName() + ">";
}
bool OptionType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const OptionType* otherOption = dynamic_cast<const OptionType*>(other)) {
        // İçteki tipler eşit mi?
         if (!InnerType || !otherOption->InnerType) return false; // Biri jenerik veya null ise eşit değil
        return InnerType->isEqual(otherOption->InnerType);
    }
    return false;
}
// TODO: OptionType için isAssignableTo, isConvertibleTo, isCopyType implementasyonları (içteki tipe bağlı)

// ResultType Implementasyonu (D++'a özgü)
std::string ResultType::getName() const {
    if (!OkType || !ErrType) return "Result<?, ?>"; // Jenerik Result tipi veya hata
    return "Result<" + OkType->getName() + ", " + ErrType->getName() + ">";
}
bool ResultType::isEqual(const Type* other) const {
    if (this == other) return true;
    if (const ResultType* otherResult = dynamic_cast<const ResultType*>(other)) {
        // İçteki Ok ve Err tipleri eşit mi?
        if (!OkType || !otherResult->OkType || !ErrType || !otherResult->ErrType) return false;
        return OkType->isEqual(otherResult->OkType) && ErrType->isEqual(otherResult->ErrType);
    }
    return false;
}
// TODO: ResultType için isAssignableTo, isConvertibleTo, isCopyType implementasyonları (içteki tiplere bağlı)


// --- TypeSystem Sınıfı Implementasyonu ---

TypeSystem::TypeSystem() {
    // İlkel Tipleri oluştur ve pointer'larını kaydet
    ErrorTy = createType<PrimitiveType>(TypeKind::Error, "<error>"); // Hata tipi özeldir
    VoidTy = createType<PrimitiveType>(TypeKind::Void, "void");
    BooleanTy = createType<PrimitiveType>(TypeKind::Boolean, "bool");
    IntTy = createType<PrimitiveType>(TypeKind::Integer, "int"); // Varsayılan tamsayı boyutu
    FloatTy = createType<PrimitiveType>(TypeKind::FloatingPoint, "float"); // Varsayılan kayan nokta boyutu
    StringTy = createType<PrimitiveType>(TypeKind::String, "string");
    CharTy = createType<PrimitiveType>(TypeKind::Character, "char");
    // TODO: Diğer ilkel tipleri oluştur (ubyte, short, ulong, float32, float64, real vb.)

    // Named types map'ine ön tanımlı tipleri ekle (Sembol tablosu da bunları kullanır)
    NamedTypes[VoidTy->getName()] = VoidTy;
    NamedTypes[BooleanTy->getName()] = BooleanTy;
    NamedTypes[IntTy->getName()] = IntTy;
    NamedTypes[FloatTy->getName()] = FloatTy;
    NamedTypes[StringTy->getName()] = StringTy;
    NamedTypes[CharTy->getName()] = CharTy;
    // TODO: Diğer ilkel tipleri NamedTypes'a ekle
}


// Tip Fabrika Metotları Implementasyonları

Type* TypeSystem::getPointerType(Type* baseType) {
    if (!isValidType(baseType)) return ErrorTy; // Geçersiz temel tip
    // Map'te var mı kontrol et
    auto it = PointerTypes.find(baseType);
    if (it != PointerTypes.end()) {
        return it->second; // Var olan pointer tipini döndür
    }
    // Yoksa yeni oluştur, kaydet ve döndür
    Type* newPtrType = createType<PointerType>(baseType);
    PointerTypes[baseType] = newPtrType;
    return newPtrType;
}

Type* TypeSystem::getReferenceType(Type* baseType, bool isMutable) {
    if (!isValidType(baseType)) return ErrorTy;
    // Map'te var mı kontrol et (temel tip ve mutability'ye göre)
    std::pair<Type*, bool> key = {baseType, isMutable};
    auto it = ReferenceTypes.find(key);
    if (it != ReferenceTypes.end()) {
        return it->second;
    }
    // Yoksa yeni oluştur, kaydet ve döndür
    Type* newRefType = createType<ReferenceType>(baseType, isMutable);
    ReferenceTypes[key] = newRefType;
    return newRefType;
}

Type* TypeSystem::getArrayType(Type* elementType, long long size) {
    if (!isValidType(elementType)) return ErrorTy;
     // Eğer size < 0 ise veya boyutu belirtilmemişse bu bir Slice tipidir.
     if (size < 0) {
          return getSliceType(elementType);
     }
    // Map'te var mı kontrol et (element tipi ve boyuta göre)
    std::pair<Type*, long long> key = {elementType, size};
    auto it = ArrayTypes.find(key);
    if (it != ArrayTypes.end()) {
        return it->second;
    }
    // Yoksa yeni oluştur, kaydet ve döndür
    Type* newArrayType = createType<ArrayType>(elementType, size);
    ArrayTypes[key] = newArrayType;
    return newArrayType;
}

Type* TypeSystem::getSliceType(Type* elementType) {
     // Slice aslında boyutu bilinmeyen bir dizidir, ArrayType'ın özel bir durumu olarak temsil edilebilir.
     // Boyut için özel bir değer kullanılır (örn: -1).
    return getArrayType(elementType, -1); // ArrayType constructor'ı Slice Kind'ı set eder
}


Type* TypeSystem::getTupleType(const std::vector<Type*>& elementTypes) {
    // Element tiplerinin hepsi geçerli mi kontrol et
    for (const auto& elemType : elementTypes) {
        if (!isValidType(elemType)) return ErrorTy;
    }
    // Map'te var mı kontrol et (element tipleri vektörüne göre)
    auto it = TupleTypes.find(elementTypes);
    if (it != TupleTypes.end()) {
        return it->second;
    }
    // Yoksa yeni oluştur, kaydet ve döndür
    Type* newTupleType = createType<TupleType>(elementTypes);
    TupleTypes[elementTypes] = newTupleType;
    return newTupleType;
}

Type* TypeSystem::getFunctionType(Type* returnType, const std::vector<Type*>& parameterTypes) {
     if (!isValidType(returnType)) return ErrorTy;
     for (const auto& paramType : parameterTypes) {
        if (!isValidType(paramType)) return ErrorTy;
     }

    // Map'te var mı kontrol et (dönüş tipi ve parametre tipleri vektörüne göre)
    std::pair<Type*, std::vector<Type*>> key = {returnType, parameterTypes};
    auto it = FunctionTypes.find(key);
    if (it != FunctionTypes.end()) {
        return it->second;
    }
    // Yoksa yeni oluştur, kaydet ve döndür
    Type* newFuncType = createType<FunctionType>(returnType, parameterTypes);
    FunctionTypes[key] = newFuncType;
    return newFuncType;
}


// Named Types (Struct, Enum, Trait) Oluşturma ve Güncelleme

StructType* TypeSystem::createStructType(const std::string& name) {
    // Bu isimde bir tip zaten var mı kontrol et (ilkel veya başka bir named type)
    if (NamedTypes.count(name)) {
        // Hata durumu: Aynı isimde başka bir tip zaten tanımlanmış.
        // Bu kontrol Sema sınıfında SymbolTable kullanılırken de yapılabilir, ama burada da yapmak iyi olabilir.
        // Sema'nın SymbolTable'a eklemeden önce NamedTypes'ta bu ismi kontrol etmesi daha temizdir.
        // Şimdilik sadece yeni bir tip oluşturup NamedTypes'a ekleyelim, çift isim hatası Sema'da raporlansın.
    }

    // Yeni StructType oluştur
    StructType* newStruct = createType<StructType>(name);
    // NamedTypes map'ine ekle
    NamedTypes[name] = newStruct;
    return newStruct;
}

void TypeSystem::updateStructType(StructType* structType, std::vector<StructField> fields) {
    if (!structType || structType->getKind() != TypeKind::Struct) return;
    // StructType nesnesinin alanlarını set et
    structType->setFields(std::move(fields));
    // TODO: Alan tiplerinin geçerli olduğunu kontrol et.
}

EnumType* TypeSystem::createEnumType(const std::string& name) {
    if (NamedTypes.count(name)) { /* Hata kontrolü */ }
    EnumType* newEnum = createType<EnumType>(name);
    NamedTypes[name] = newEnum;
    return newEnum;
}

void TypeSystem::updateEnumType(EnumType* enumType, std::vector<EnumVariant> variants) {
     if (!enumType || enumType->getKind() != TypeKind::Enum) return;
     enumType->setVariants(std::move(variants));
     // TODO: Varyant detaylarını (değerler, tipler) kontrol et.
}

TraitType* TypeSystem::createTraitType(const std::string& name) {
     if (NamedTypes.count(name)) { /* Hata kontrolü */ }
     TraitType* newTrait = createType<TraitType>(name);
     NamedTypes[name] = newTrait;
     return newTrait;
}

void TypeSystem::updateTraitType(TraitType* traitType, std::vector<TraitMethod> methods) {
     if (!traitType || traitType->getKind() != TypeKind::Trait) return;
     traitType->setRequiredMethods(std::move(methods));
     // TODO: Metot imzalarının geçerli olduğunu kontrol et.
}


Type* TypeSystem::getNamedType(const std::string& name) const {
    auto it = NamedTypes.find(name);
    if (it != NamedTypes.end()) {
        return it->second;
    }
    return nullptr; // Bulunamadı
}


// D++ Özgü Tip Fabrika Metotları Implementasyonları

Type* TypeSystem::getOptionType(Type* innerType) {
    // Eğer innerType nullptr ise, jenerik Option tipini temsil etmek için özel bir Type* kullanabilirsiniz.
    // Şimdilik sadece instancelar için implementasyon yapalım.
    if (!isValidType(innerType)) {
         // Eğer innerType hata ise, sonuç ErrorType olmalı.
         // Eğer innerType null ise, bu jenerik tipi mi temsil ediyor? Kullanımına bağlı.
         // Şimdilik null innerType'a izin vermeyelim.
         return ErrorTy;
    }

    // Map'te var mı kontrol et
    auto it = OptionTypes.find(innerType);
    if (it != OptionTypes.end()) {
        return it->second;
    }
    // Yoksa yeni oluştur, kaydet ve döndür
    Type* newOptionType = createType<OptionType>(innerType);
    OptionTypes[innerType] = newOptionType;
    return newOptionType;
}

Type* TypeSystem::getResultType(Type* okType, Type* errType) {
     if (!isValidType(okType) || !isValidType(errType)) {
         // Eğer tiplerden biri hata ise, sonuç ErrorType olmalı.
         return ErrorTy;
     }

    // Map'te var mı kontrol et
    std::pair<Type*, Type*> key = {okType, errType};
    auto it = ResultTypes.find(key);
    if (it != ResultTypes.end()) {
        return it->second;
    }
    // Yoksa yeni oluştur, kaydet ve döndür
    Type* newResultType = createType<ResultType>(okType, errType);
    ResultTypes[key] = newResultType;
    return newResultType;
}


// --- Tip Karşılaştırma ve Sorgulama Metotları Implementasyonları ---

bool TypeSystem::isEqual(const Type* type1, const Type* type2) const {
    if (type1 == type2) return true; // Pointer eşitliği kontrolü hızlıdır
    if (!type1 || !type2) return false;
     // Hata tipleri sadece kendilerine eşit olabilir (pointer bazında)
    if (type1->isErrorType() || type2->isErrorType()) return false;

    // Sanal metoda devret
    return type1->isEqual(type2);
}

bool TypeSystem::isAssignable(const Type* destType, const Type* srcType) const {
    if (!isValidType(destType) || !isValidType(srcType)) return false;

    // Sanal metoda devret
    return srcType->isAssignableTo(destType);

    // TODO: Genel atanabilirlik kuralları burada implement edilebilir.
    // Örneğin: srcType'ın destType'a implicit dönüşümü var mı?
    // Sayısal genişleme (int -> float), referans dönüştürme (&mut T -> &T), pointer dönüştürme (*T -> *const T)
    // Base class pointer = Derived class pointer (Subtyping)
    // Bu kısım dilinizin anlamsal kurallarını yansıtır.
}

bool TypeSystem::isConvertible(const Type* fromType, const Type* toType) const {
     if (!isValidType(fromType) || !isValidType(toType)) return false;

     // Sanal metoda devret
     return fromType->isConvertibleTo(toType);

     // TODO: Genel dönüştürülebilirlik (explicit cast) kuralları burada implement edilebilir.
     // Örneğin: Sayısal tipler arası (daralma dahil), pointer <-> integer, pointer <-> pointer.
     // Bu kısım dilinizin anlamsal kurallarını yansıtır ve isAssignable'dan daha gevşek kurallar içerebilir.
}

bool TypeSystem::isComparable(const Type* type1, const Type* type2) const {
    if (!isValidType(type1) || !isValidType(type2)) return false;
    // Sanal metoda devret (iki yönde de kontrol edebiliriz)
     return type1->isComparableWith(type2) || type2->isComparableWith(type1);
    // Genellikle karşılaştırılabilir olmak simetriktir.
}

Type* TypeSystem::getWiderNumericType(const Type* type1, const Type* type2) const {
    if (!isValidType(type1) || !isValidType(type2) || !type1->isNumericType() || !type2->isNumericType()) {
        return ErrorTy; // Sayısal olmayan tipler veya hatalı tipler için ortak geniş tip yok
    }

    if (type1->isEqual(type2)) return const_cast<Type*>(type1); // Aynı tiplerin ortak geniş tipi kendileridir

    // TODO: Sayısal tip hiyerarşisine göre geniş olanı belirle.
    // Örneğin: int < long < float < double < real
    // Bu logic TypeSystem'ın sayısal tiplerin boyutlarını veya rangelerini bilmesini gerektirir.
    // PrimitiveType sınıfına size/range bilgisi eklenebilir veya burada hardcode edilebilir.
    // Basit bir örnek: int ve float -> float
    if (type1->isFloatingPointType() || type2->isFloatingPointType()) {
         // Eğer bir tanesi bile float ise sonuç float'tır (varsayılan olarak en geniş kayan nokta)
         // Daha detaylı karşılaştırma yapılabilir (float32 vs float64)
         return FloatTy; // Veya daha spesifik bir FloatType*
    }
    if (type1->isIntegerType() && type2->isIntegerType()) {
         // İki integer ise, geniş olan integer tipi döndürülür (long vs int)
         // TODO: Integer tiplerin genişliğini karşılaştır.
          return Types.getWiderIntegerType(type1, type2);
         return IntTy; // Varsayılan olarak int döndürelim (basitlik için)
    }

    // Beklenmedik durum, hata
    return ErrorTy;
}

// Debug için tüm oluşturulmuş tipleri listele
void TypeSystem::dumpCreatedTypes() const {
     std::cerr << "--- Created Types ---\n";
     for (const auto& uniquePtr : AllTypes) {
         const Type* type = uniquePtr.get();
         if (type) {
              std::cerr << "Kind: " << static_cast<int>(type->getKind()) << ", Name: " << type->getName() << ", Addr: " << type << "\n";
         }
     }
     std::cerr << "---------------------\n";
}


} // namespace sema
} // namespace dppc
