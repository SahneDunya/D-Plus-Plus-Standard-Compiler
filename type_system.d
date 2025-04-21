module type_system;

import std.stdio;
import types;       // DppType ve ilgili yapılar
import syntax_kind; // Operatörler için SyntaxKind
import ast;         // AST düğümleri (tip çıkarımı veya hata raporlama için gerekebilir)
import error_reporting; // Hata raporlama için


// İki türün atanabilirlik uyumluluğunu kontrol eder (Type Coercion kurallarını içerir)
// Örneğin, int'in float'a atanabilirliği gibi.
bool isAssignableTo(const DppType* source, const DppType* target) {
    if (!source || !target) {
        // Hata durumu
        return false;
    }

    // 1. Tam Eşitlik
    if (source.isExactly(target)) {
        return true;
    }

    // 2. Temel Tip Uyumluluğu ve Yükseltme (Coercion)
    // Örnek: int -> float atamasına izin verelim
    if (source.kind == DppTypeKind.Primitive && target.kind == DppTypeKind.Primitive) {
        if (source == IntType && target == FloatType) {
            writeln("Tip yükseltme: int -> float");
            return true; // int'ten float'a atama serbest
        }
        // Diğer temel tipler arası yükseltme kuralları buraya eklenecek.
    }

    // 3. Referans ve İşaretçi Uyumluluğu (Karmaşık kurallar)
    // Rust'taki ödünç alma kuralları burada kontrol edilmelidir.
    // Örneğin, &mut T'yi &T'ye atamak serbesttir, ancak tersi değildir.
    // İşaretçiler ve referanslar arasındaki atama kuralları.
    if (source.kind == DppTypeKind.Reference && target.kind == DppTypeKind.Reference) {
        auto sourceRef = cast(const(ReferenceType))source;
        auto targetRef = cast(const(ReferenceType))target;
        // Temel tipleri aynıysa
        if (sourceRef.baseType.isExactly(targetRef.baseType)) {
            // Immutable referansı Immutable veya Mutable referansa atayabiliriz.
            // Mutable referansı sadece Mutable referansa atayabiliriz.
            // Rust kuralı: &mut T sadece &mut T'ye, &T hem &T hem &mut T'ye (mutable borrow yoksa) atanabilir.
            if (!sourceRef.isMutable || targetRef.isMutable) {
                writeln("Referans atama uyumluluğu kontrolü (Rust benzeri)");
                // Tam kontrol için ödünç alma kuralları ve yaşam süreleri de burada olmalı (karmaşık!)
                 return true; // Basit kural: mut -> mut veya immut -> (mut veya immut)
            }
        }
    }
    // İşaretçi atama kuralları (örneğin, *T'yi *void'e atama)

    // 4. Kullanıcı Tanımlı Türler Arası Uyumluluk (Kalıtım, Arayüzler vb.)
    // Eğer dil kalıtımı destekliyorsa, alt sınıfın üst sınıfa atanabilirliği burada kontrol edilir.
    // Arayüz implementasyonları da burada kontrol edilir.

    // 5. Fonksiyon İşaretçisi/Referansı Uyumluluğu (Karmaşık)
    // Fonksiyon imzalarının (parametre ve dönüş tipleri) uyumluluğu.

    // Varsayılan olarak atanabilir değil
    return false;
}

// İki türün eşit olup olmadığını kontrol eder (Genellikle tam eşitlik veya belirli uyumluluk türleri)
// Bu, types.d'deki isExactly fonksiyonunu çağırabilir veya daha gevşek kurallar içerebilir.
bool areTypesEqual(const DppType* type1, const DppType* type2) {
    if (!type1 || !type2) return false;
    return type1.isExactly(type2); // Varsayılan olarak tam eşitliği kullanalım
    // veya daha gevşek bir kural: return isAssignableTo(type1, type2) && isAssignableTo(type2, type1);
}


// İkili operatörün işlenen tiplerine göre sonuç tipini belirler ve uyumluluk hatalarını kontrol eder.
// Bu fonksiyon type_checker.d'dekinden daha detaylı olabilir.
DppType* getBinaryOperatorResultType(SyntaxKind operatorKind, const DppType* leftType, const DppType* rightType) {
    if (!leftType || !rightType) return DppType.Unknown;

    switch (operatorKind) {
        // Aritmetik Operatörler (+, -, *, /)
        case SyntaxKind.PlusToken:
        case SyntaxKind.MinusToken:
        case SyntaxKind.StarToken:
        case SyntaxKind.SlashToken:
            // Int + Int -> Int
            if (leftType == IntType && rightType == IntType) return IntType;
            // Float + Float -> Float
            if (leftType == FloatType && rightType == FloatType) return FloatType;
            // Int + Float veya Float + Int -> Float (Tip yükseltme)
            if ((leftType == IntType && rightType == FloatType) || (leftType == FloatType && rightType == IntType)) {
                 writeln("Tip yükseltme: Aritmetik işlemde Int ve Float."); // Uyarı verilebilir
                 return FloatType;
            }
            // String birleştirme (+)
            if (operatorKind == SyntaxKind.PlusToken && leftType == StringType && rightType == StringType) return StringType;

            // Uyumsuz tipler
             stderr.writeln("Hata (TypeSystem): Uyumsuz tiplerde ('", leftType.name, "' ve '", rightType.name, "') aritmetik işlem.");
            return DppType.Unknown;

        // Karşılaştırma Operatörleri (==, !=, <, <=, >, >=)
        case SyntaxKind.EqualsEqualsToken:
        case SyntaxKind.BangEqualsToken:
        case SyntaxKind.LessThanToken:
        case SyntaxKind.LessThanEqualsToken:
        case SyntaxKind.GreaterThanToken:
        case SyntaxKind.GreaterThanEqualsToken:
            // İşlenen tipleri uyumlu olmalı ve sonuç boolean olmalı.
            if (areTypesEqual(leftType, rightType) || (leftType == IntType && rightType == FloatType) || (leftType == FloatType && rightType == IntType)) {
                // Sayısal veya boolean tipler karşılaştırılabilir
                if (leftType == IntType || leftType == FloatType || leftType == BoolType) {
                    return BoolType;
                }
                // String karşılaştırması (==, !=)
                if (operatorKind == SyntaxKind.EqualsEqualsToken || operatorKind == SyntaxKind.BangEqualsToken) {
                    if (leftType == StringType && rightType == StringType) return BoolType;
                }

                 stderr.writeln("Uyarı (TypeSystem): Bu tipler ('", leftType.name, "') için karşılaştırma operatörü ('", operatorKind.to!string, "') tam desteklenmiyor."); // Uyarı verilebilir
                 return BoolType; // Varsayılan olarak boolean dönelim
            }
            // Uyumsuz tipler
            stderr.writeln("Hata (TypeSystem): Uyumsuz tiplerde ('", leftType.name, "' ve '", rightType.name, "') karşılaştırma işlemi.");
            return DppType.Unknown;

        // Mantıksal Operatörler (&&, ||)
         case SyntaxKind.AmpersandAmpersandToken:
         case SyntaxKind.PipePipeToken:
        //     // İşlenen tipleri boolean olmalı ve sonuç boolean olmalı.
             if (leftType == BoolType && rightType == BoolType) return BoolType;
             stderr.writeln("Hata (TypeSystem): Mantıksal operatörler için işlenen tipleri boolean olmalı.");
             return DppType.Unknown;


        // ... Diğer operatör türleri için sonuç tipi ve uyumluluk kuralları

        default:
             stderr.writeln("Hata (TypeSystem): Bilinmeyen operatör türü için sonuç tipi belirleme: ", operatorKind.to!string);
            return DppType.Unknown;
    }
}

// Tip çıkarımı (Type Inference) fonksiyonu (basit bir taslak)
// Genellikle bir ifadenin AST düğümünü alır ve bu ifadenin belirlenen tipini döndürür.
// AST düğümlerinin üzerinde dolaşarak recursive olarak tipleri belirler.
// Bu fonksiyon, type_checker.d'deki checkType fonksiyonu ile yakın ilişkili veya aynı olabilir.
 DppType* inferType(ASTNode* expressionNode, SymbolTable* currentScope);


// Diğer tip sistemi kuralları ve yardımcı fonksiyonlar
// - Alt tür ilişkileri kontrolü
// - Fonksiyon imza uyumluluğu kontrolü
// - Kullanıcı tanımlı türlerin (struct, class) tip kontrolü
// - Jenerik (generic) tiplerin işlenmesi (karmaşık!)