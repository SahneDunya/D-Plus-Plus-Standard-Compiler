module operator_analyzer;

import std.stdio;
import ast;           // AST düğüm yapıları (BinaryOperatorExpressionNode, UnaryOperatorExpressionNode)
import syntax_kind;   // SyntaxKind enum'ı
import dpp_types;     // D++ tür temsilcileri
import type_system;   // Tür sistemi kuralları (isAssignableTo, getBinaryOperatorResultType vb.)
import scope_manager; // Kapsam bilgisi için
import error_reporting; // Hata raporlama için

// İkili operatör ifadesini anlamsal olarak analiz eden fonksiyon
// Genellikle semantic_analyzer'daki AST traversi sırasında BinaryOperatorExpressionNode ile karşılaşınca çağrılır.
// İfadenin belirlenen tipini döndürür ve hata varsa raporlar.
DppType* analyzeBinaryOperator(BinaryOperatorExpressionNode* node, ScopeManager* scopeManager) {
    if (!node || !node.left || !node.right) {
        // Hata: Geçersiz ikili operatör düğümü
        stderr.writeln("Hata (OperatorAnalyzer): Geçersiz ikili operatör düğümü.");
         error_reporting.reportError(...);
        return DppType.Unknown;
    }

    // Sol ve sağ işlenenleri recursive olarak analiz et ve tiplerini belirle
    // Bu, semantic_analyzer'daki traverseAST veya type_checker'daki checkType tarafından zaten yapılmış olabilir.
    // Burada sadece belirlenmiş tipleri aldığımızı varsayalım.
     DppType* leftType = analyzeExpression(node.left, scopeManager); // Eğer analyzeExpression varsa
     DppType* rightType = analyzeExpression(node.right, scopeManager); // Eğer analyzeExpression varsa

    // Basitlik için type_checker'dan tipleri alalım
     DppType* leftType = checkType(node.left, scopeManager.getCurrentScope()); // type_checker.d'deki fonksiyon
     DppType* rightType = checkType(node.right, scopeManager.getCurrentScope()); // type_checker.d'deki fonksiyon


    // Operatör ve işlenen tiplerine göre sonuç tipini belirle ve uyumluluğu kontrol et
    // Bu mantık type_system.d'de yer almalıdır.
     DppType* resultType = getBinaryOperatorResultType(node.operatorToken.kind, leftType, rightType); // type_system.d'deki fonksiyon

    if (resultType.kind == DppTypeKind.Unknown) {
        // Hata, getBinaryOperatorResultType içinde raporlanmış olmalı, ama burada da teyit edelim.
         stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": İkili operatör ('", node.operatorToken.value, "') için tip hatası.");
         error_reporting.reportError(...);
         errors++; // Hata sayısını artır (çağıran fonksiyon yapabilir)
    }

    // Atama operatörü için özel kontrol (sol tarafın atanabilir olup olmadığı)
    if (node.operatorToken.kind == SyntaxKind.EqualsToken) {
         // Sol taraf bir değişken, üye erişimi veya dizi erişimi olmalı (l-value)
         // Sol tarafın tipinin belirlenmiş olması ve mutable olması gerekir.
         bool isAssignable = checkIsAssignable(node.left, scopeManager); // Aşağıda tanımlanacak yardımcı fonksiyon
         if (!isAssignable) {
              stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": Atamanın sol tarafı atanabilir bir ifade değil.");
              error_reporting.reportError(...);
              errors++;
         }
         // Atama operatörünün tipi genellikle atanan değerin tipidir veya void'dir (dil kurallarına bağlı).
         // Rust'ta atama bir ifade değildir (statement'tır) ve değer döndürmez. D++'ta nasıl olacak?
         // Eğer ifadeyse, genellikle sağ tarafın tipini döndürür.
         return rightType; // Sağ tarafın tipini döndürelim (basitlik için)
          Veya return DppType.VoidType; // Eğer atama bir statement ise ve değer döndürmüyorsa
    }


    // İfadenin çözümlenmiş tipini AST düğümüne ekle (gelişmiş AST)
    // node.resolvedType = resultType;

    return resultType; // İfadenin belirlenen tipini döndür
}

// Tekli operatör ifadesini anlamsal olarak analiz eden fonksiyon
// Genellikle semantic_analyzer'daki AST traversi sırasında UnaryOperatorExpressionNode ile karşılaşınca çağrılır.
DppType* analyzeUnaryOperator(UnaryOperatorExpressionNode* node, ScopeManager* scopeManager) {
    if (!node || !node.operand) {
        // Hata: Geçersiz tekli operatör düğümü
        stderr.writeln("Hata (OperatorAnalyzer): Geçersiz tekli operatör düğümü.");
         error_reporting.reportError(...);
        return DppType.Unknown;
    }

    // İşleneni recursive olarak analiz et ve tipini belirle
     DppType* operandType = checkType(node.operand, scopeManager.getCurrentScope()); // type_checker.d'deki fonksiyon

    // Operatör ve işlenen tipine göre sonuç tipini belirle ve uyumluluğu kontrol et
    // Bu mantık type_system.d'de yer almalıdır (yeni bir fonksiyon gerektirebilir).
     DppType* resultType = getUnaryOperatorResultType(node.operatorToken.kind, operandType); // type_system.d'de tanımlanacak

    switch (node.operatorToken.kind) {
        case SyntaxKind.MinusToken: // Negatif işaret (-)
        case SyntaxKind.PlusToken:  // Tekli +
            if (operandType == IntType || operandType == FloatType) return operandType; // Sonuç işlenenle aynı tip
            stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": Sayısal olmayan tipte ('", operandType.name, "') tekli '+' veya '-' operatörü.");
            return DppType.Unknown;

        case SyntaxKind.BangToken: // Mantıksal NOT (!)
            if (operandType == BoolType) return BoolType; // Sonuç boolean
            stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": Boolean olmayan tipte ('", operandType.name, "') tekli '!' operatörü.");
            return DppType.Unknown;

        case SyntaxKind.AmpersandToken: // Referans al (&)
            // Bu, bir referans türü döndürür. İşlenen atanabilir olmalıdır.
             bool isReferencable = checkIsAssignable(node.operand, scopeManager); // Atanabilir ise referans alınabilir varsayalım
             if (!isReferencable) {
                  stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": Referansı alınamayan bir ifadenin referansı alınıyor.");
                  error_reporting.reportError(...);
                 return DppType.Unknown;
             }
            // Yeni bir referans türü oluştur (Mutable mi immutable mı operatorToken'a bağlı)
             bool isMutableRef = (node.operatorToken.kind == SyntaxKind.AmpersandAmpersandToken); // Varsayım: && mut referans al
             return new ReferenceType(operandType, isMutableRef); // types.d'deki sınıf

        case SyntaxKind.StarToken: // Dereference (*)
            // İşlenen bir işaretçi veya referans türü olmalıdır.
            if (operandType.kind == DppTypeKind.Pointer) {
                 auto ptrType = cast(PointerType*)operandType;
                 return ptrType.baseType; // İşaret edilen türü döndür
            }
            if (operandType.kind == DppTypeKind.Reference) {
                 auto refType = cast(ReferenceType*)operandType;
                 return refType.baseType; // Referans verilen türü döndür
            }
            stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": İşaretçi veya referans olmayan tipte ('", operandType.name, "') dereference operatörü ('*').");
            return DppType.Unknown;

        // ... Diğer tekli operatörler

        default:
             stderr.writeln("Hata (OperatorAnalyzer): Bilinmeyen tekli operatör türü için analiz: ", node.operatorToken.kind.to!string);
            return DppType.Unknown;
    }

    // İfadenin çözümlenmiş tipini AST düğümüne ekle (gelişmiş AST)
     node.resolvedType = resultType;

     return resultType; // Belirlenen tipi döndür
}


// Bir ifadenin atanabilir (l-value) olup olmadığını kontrol eden yardımcı fonksiyon
// Değişkenler, üye erişimleri, dizi/index erişimleri genellikle atanabilirdir.
// Literaller, sabitler, fonksiyon çağrıları genellikle atanabilir değildir.
bool checkIsAssignable(ASTNode* node, ScopeManager* scopeManager) {
    if (!node) return false;

    switch (node.kind) {
        case SyntaxKind.IdentifierExpressionNode:
            // Bir değişkenin adı. Sembol tablosunda değişken olarak tanımlanmış ve mutable mı kontrol et.
            auto identNode = cast(IdentifierExpressionNode*)node;
            Symbol* symbol = scopeManager.resolveSymbol(identNode.value);
            if (symbol && symbol.kind == SyntaxKind.IdentifierToken) { // Eğer değişken ise
                return symbol.isMutable; // Mutable ise atanabilir
            }
            return false; // Sembol bulunamadı veya değişken değil

        case SyntaxKind.MemberAccessExpressionNode: // obj.field
            // Üye erişiminin sonucu atanabilir mi kontrol et (üyenin mutable olup olmadığına bağlı olabilir)
            // Sol tarafın (obj) tipini belirle ve üyenin mutable olup olmadığını kontrol et.
            // Bu daha karmaşık bir tip sistemi ve üye arama gerektirir.
             stderr.writeln("Uyarı: Üye erişimi atanabilirlik kontrolü implemente edilmedi.");
             return false; // Placeholder

        case SyntaxKind.IndexAccessExpressionNode: // arr[i]
            // Dizi/index erişiminin sonucu atanabilir mi kontrol et (dizinin mutable olup olmadığına bağlı)
            // Sol tarafın (arr) tipini belirle ve mutable olup olmadığını kontrol et.
             stderr.writeln("Uyarı: Index erişimi atanabilirlik kontrolü implemente edilmedi.");
             return false; // Placeholder

        case SyntaxKind.DereferenceExpressionNode: // *ptr
             // Dereference edilen ifade atanabilir mi kontrol et (işaret edilen yerin mutable olup olmadığına bağlı)
             // İşlenenin tipini belirle (*T veya &T) ve T'nin mutable olup olmadığını kontrol et.
              stderr.writeln("Uyarı: Dereference atanabilirlik kontrolü implemente edilmedi.");
              return false; // Placeholder


        // ... Diğer atanabilir ifade türleri

        default:
            // Varsayılan olarak atanabilir değil
            return false;
    }
}

// Diğer yardımcı analiz fonksiyonları
// - Operatör aşırı yükleme çözümlemesi (eğer D++ destekliyorsa)
// - Özel operatörlerin (match, range operatörleri vb.) anlamsal analizi