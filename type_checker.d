module type_checker;

import std.stdio;
import ast; // AST düğüm yapıları
import syntax_kind; // SyntaxKind enum'ı
import dpp_types; // D++ tür temsilcileri
import semantic_analyzer; // SymbolTable ve Symbol yapıları için

// Bir AST ifadesinin tipini belirleyen ana fonksiyon
// Bu fonksiyon AST üzerinde recursive olarak dolaşır ve her ifadenin tipini döndürür.
DppType checkType(ASTNode* expressionNode, SymbolTable* currentScope) {
    if (!expressionNode) {
        // Hata durumu: Null ifade düğümü
        stderr.writeln("Hata: Null ifade için tip kontrolü yapılıyor.");
         error_reporting.reportError(...);
        return DppType.Unknown;
    }

    // İfade düğümünün türüne göre tip belirle
    switch (expressionNode.kind) {
        case SyntaxKind.IntegerLiteralToken: // LiteralExpressionNode'un alt türü olabilir
        case SyntaxKind.FloatingPointLiteralToken:
        case SyntaxKind.StringLiteralToken:
        case SyntaxKind.BooleanLiteralToken:
            auto literalNode = cast(LiteralExpressionNode*)expressionNode;
            // Literal token türüne göre D++ tipini döndür
            switch (literalNode.literalToken.kind) {
                case SyntaxKind.IntegerLiteralToken: return IntType;
                case SyntaxKind.FloatingPointLiteralToken: return FloatType;
                case SyntaxKind.StringLiteralToken: return StringType;
                case SyntaxKind.BooleanLiteralToken: return BoolType;
                default: return DppType.Unknown; // Bilinmeyen literal tipi
            }

        case SyntaxKind.IdentifierExpressionNode:
            auto identNode = cast(IdentifierExpressionNode*)expressionNode;
            // Tanımlayıcının sembol tablosundaki tipini döndür (İsim Çözümlemesi daha önce yapılmış olmalı)
            Symbol* symbol = currentScope.resolve(identNode.value, false);
            if (symbol) {
                return symbol.type;
            } else {
                // Sembol bulunamadı (Semantik analizde hata verilmiş olmalı)
                return DppType.Unknown;
            }

        case SyntaxKind.BinaryOperatorExpressionNode:
            auto binOpNode = cast(BinaryOperatorExpressionNode*)expressionNode;
            // Sol ve sağ işlenenlerin tiplerini belirle (recursive çağrı)
            DppType leftType = checkType(binOpNode.left, currentScope);
            DppType rightType = checkType(binOpNode.right, currentScope);

            // Operatör ve işlenen tiplerine göre sonuç tipini belirle ve uyumluluğu kontrol et
            return checkBinaryOperatorType(binOpNode, leftType, rightType); // Bu fonksiyon aşağıda tanımlanacak

        case SyntaxKind.CallExpressionNode:
             auto callNode = cast(CallExpressionNode*)expressionNode;
            // Fonksiyon çağrısının tipini belirle
            // 1. Çağrılan fonksiyonun tipini belirle (checkType ile IdentifierExpressionNode olarak işlenecek)
            DppType functionType = checkType(callNode.function, currentScope); // callNode.function bir IdentifierExpressionNode olabilir

            // 2. Argümanların tiplerini belirle
            DppType[] argumentTypes;
            foreach (arg; callNode.arguments) {
                argumentTypes ~= checkType(arg, currentScope);
            }

            // 3. Fonksiyon tipinin argüman tipleriyle uyumlu olup olmadığını kontrol et
            // Bu kısım, fonksiyon tipinin yapısını (parametre tipleri ve dönüş tipi) bilmeyi gerektirir.
            // Fonksiyon tipinin DppType yapısında detaylı bilgi olması gerekir.
             stderr.writeln("Uyarı: Fonksiyon çağrısı tip kontrolü implemente edilmedi."); // Placeholder
             return DppType.Unknown; // Placeholder

            // Eğer uyumluysa, fonksiyonun dönüş tipini döndür. Uyumlu değilse hata ver ve UnknownType döndür.
            return functionType.returnType; // Varsayım: FunctionType DppType içinde

        // ... Diğer ifade türleri için tip kontrolü

        default:
            // Bu bir ifade düğümü değilse veya tip kontrolü yapılmayacaksa Unknown döndür
            stderr.writeln("Uyarı: Tip kontrolü yapılmayan düğüm türü: ", expressionNode.kind.to!string);
            return DppType.Unknown;
    }
}

// İkili operatör ifadeleri için özel tip kontrol fonksiyonu
// Sol ve sağ işlenenlerin tiplerini ve operatörü alarak sonuç tipini belirler ve uyumluluk hatalarını raporlar.
DppType checkBinaryOperatorType(BinaryOperatorExpressionNode* binOpNode, DppType leftType, DppType rightType) {
    // Basitlik için sadece int ve float için temel aritmetik ve karşılaştırma operatörlerini ele alalım.
    // Gerçek bir dilde tip yükseltme (type coercion), operatör aşırı yükleme (operator overloading) gibi kurallar işin içine girer.

    switch (binOpNode.operatorToken.kind) {
        // Aritmetik Operatörler (+, -, *, /)
        case SyntaxKind.PlusToken:
        case SyntaxKind.MinusToken:
        case SyntaxKind.StarToken:
        case SyntaxKind.SlashToken:
            // Eğer her iki işlenen de int ise sonuç int'tir.
            if (leftType.kind == TypeKind.Int && rightType.kind == TypeKind.Int) {
                return IntType;
            }
            // Eğer her iki işlenen de float ise sonuç float'tır.
            if (leftType.kind == TypeKind.Float && rightType.kind == TypeKind.Float) {
                return FloatType;
            }
            // Eğer biri int diğeri float ise, tip yükseltme olabilir ve sonuç float olur (Dil kurallarına bağlı)
            if ((leftType.kind == TypeKind.Int && rightType.kind == TypeKind.Float) ||
                (leftType.kind == TypeKind.Float && rightType.kind == TypeKind.Int)) {
                 stderr.writeln("Uyarı: Satır ", binOpNode.operatorToken.lineNumber, ": Int ve Float arasında tip yükseltme yapılıyor."); // Uyarı verilebilir
                 return FloatType; // Sonuç tipi float olsun
            }
            // Uyumsuz tiplerde hata
            stderr.writeln("Hata: Satır ", binOpNode.operatorToken.lineNumber, ": Uyumsuz tiplerde ('", leftType.name, "' ve '", rightType.name, "') aritmetik işlem ('", binOpNode.operatorToken.value, "').");
             error_reporting.reportError(...);
            return DppType.Unknown;

        // Karşılaştırma Operatörleri (==, !=, <, <=, >, >=)
        case SyntaxKind.EqualsEqualsToken:
        case SyntaxKind.BangEqualsToken:
        case SyntaxKind.LessThanToken:
        case SyntaxKind.LessThanEqualsToken:
        case SyntaxKind.GreaterThanToken:
        case SyntaxKind.GreaterThanEqualsToken:
            // Karşılaştırma operatörleri için işlenen tipleri aynı veya uyumlu olmalı ve sonuç tipi boolean'dır.
            if (leftType.isEquivalent(rightType)) {
                // Sayısal tipler veya boolean tipleri karşılaştırılabilir (dil kurallarına bağlı)
                if (leftType.kind == TypeKind.Int || leftType.kind == TypeKind.Float || leftType.kind == TypeKind.Bool) {
                    return BoolType; // Sonuç her zaman boolean
                }
                 // Diğer tipler için karşılaştırma kuralları (string karşılaştırması gibi)
                 stderr.writeln("Uyarı: Satır ", binOpNode.operatorToken.lineNumber, ": Bu tip ('", leftType.name, "') için karşılaştırma operatörü ('", binOpNode.operatorToken.value, "') henüz tam desteklenmiyor."); // Uyarı verilebilir
                 return BoolType; // Varsayılan olarak boolean dönelim
            }
            // Uyumsuz tiplerde hata
            stderr.writeln("Hata: Satır ", binOpNode.operatorToken.lineNumber, ": Uyumsuz tiplerde ('", leftType.name, "' ve '", rightType.name, "') karşılaştırma işlemi ('", binOpNode.operatorToken.value, "').");
             error_reporting.reportError(...);
            return DppType.Unknown;

        // ... Diğer operatör türleri için tip kontrolü (Mantıksal, bitwise, atama vb.)
        default:
            stderr.writeln("Hata: Satır ", binOpNode.operatorToken.lineNumber, ": Bilinmeyen ikili operatör türü için tip kontrolü: '", binOpNode.operatorToken.value, "' (", binOpNode.operatorToken.kind, ")");
             error_reporting.reportError(...);
            return DppType.Unknown;
    }
}

// Atama ifadeleri için tip kontrolü (Örnek: sol tarafın tipi sağ tarafın tipiyle uyumlu mu?)
 void checkAssignmentType(AssignmentExpressionNode* assignNode, SymbolTable* currentScope) {
     DppType leftType = checkType(assignNode.left, currentScope);
     DppType rightType = checkType(assignNode.right, currentScope);

     if (!leftType.isEquivalent(rightType)) {
         stderr.writeln("Hata: Satır ", assignNode.operatorToken.lineNumber, ": Atama uyumsuzluğu. Sol taraf tipi ('", leftType.name, "') sağ taraf tipiyle ('", rightType.name, "') uyumlu değil.");
         error_reporting.reportError(...);
     }
     // Ayrıca sol tarafın atanabilir (örneğin, değişken olması ve mutable olması) olup olmadığı kontrol edilmelidir.
 }

// Diğer tip kontrolü yardımcı fonksiyonları (örneğin, fonksiyon argümanları ile parametre tiplerinin uyumluluğu)