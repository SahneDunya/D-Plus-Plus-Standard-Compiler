module variable_manager;

import std.stdio;
import ast;           // AST düğüm yapıları (VariableDeclarationNode gibi)
import syntax_kind;   // SyntaxKind enum'ı
import dpp_types;     // D++ tür temsilcileri
import symbol_table;  // Symbol yapısı
import scope_manager; // Kapsam yönetimi için
import type_checker;  // Tip çıkarımı için

// Bir değişken bildirimini işleyen fonksiyon
// Genellikle semantic_analyzer'daki AST traversi sırasında VariableDeclarationNode ile karşılaşınca çağrılır.
int processVariableDeclaration(VariableDeclarationNode* varNode, ScopeManager* scopeManager) {
    if (!varNode || !varNode.name) {
        // Hata: Geçersiz VariableDeclarationNode
        stderr.writeln("Hata (VariableManager): Geçersiz değişken bildirim düğümü.");
         error_reporting.reportError(...);
        return 1;
    }

    int errors = 0;
    string variableName = varNode.name.value;
    bool isMutable = varNode.isMutable;

    // 1. Değişkenin tipini belirle (Tip belirtilmişse kullan, yoksa çıkarım yap)
    DppType* variableType = DppType.Unknown; // Başlangıç tipi bilinmeyen

    if (varNode.type) {
        // Tip belirtimi varsa, ilgili DppType nesnesini al
        // Bu kısım, TypeNode AST düğümünden DppType'a çevirme mantığı gerektirir.
        // Örneğin: variableType = getTypeFromTypeNode(varNode.type);
         stderr.writeln("Uyarı (VariableManager): Tip belirtimi çözme implemente edilmedi."); // Placeholder
         variableType = DppType.IntType; // Geçici olarak int varsayalım
    } else if (varNode.initializer) {
        // Tip belirtilmemişse ve başlangıç değeri varsa, tip çıkarımı yap
         variableType = checkType(varNode.initializer, scopeManager.getCurrentScope()); // TypeChecker kullan

        // Eğer tip çıkarılamadıysa ve tip belirtilmemişse hata
        if (variableType.kind == DppTypeKind.Unknown) {
             stderr.writeln("Hata: Satır ", varNode.name.identifierToken.lineNumber, ": Değişken tipi belirtilmeli veya başlangıç değerinden çıkarılabilmeli.");
              error_reporting.reportError(...);
             errors++;
        }
    } else {
        // Hem tip belirtilmemiş hem de başlangıç değeri yoksa hata (Çoğu dilde geçerli bir bildirim değildir)
        stderr.writeln("Hata: Satır ", varNode.name.identifierToken.lineNumber, ": Değişken bildiriminde tip belirtilmeli veya başlangıç değeri atanmalı.");
         error_reporting.reportError(...);
        errors++;
        variableType = DppType.Unknown; // Tipini bilinmeyen olarak ayarla
    }


    // Eğer tip Unknown değilse ve hata yoksa devam et
    if (variableType.kind != DppTypeKind.Unknown && errors == 0) {
        // 2. Sembol tablosuna ekle (Mevcut kapsamda)
        // ScopeManager zaten aynı isimde sembol varsa hata verecektir.
        Symbol newSymbol = {
            name: variableName,
            type: variableType,
            kind: SyntaxKind.IdentifierToken, // Değişken sembolleri için IdentifierToken kullanılabilir
            isMutable: isMutable
             declarationNode: varNode // İsteğe bağlı olarak AST düğümüne referans
        };

        if (!scopeManager.addSymbol(newSymbol)) {
            // ScopeManager hata raporladıysa, burada ek bir şey yapmaya gerek yok, sadece hata sayısını artır.
            errors++;
        } else {
             // Başlangıç değeri ifadesini işle ve tipini kontrol et (Semantik analizde zaten yapılıyor olabilir, ama burada da teyit edilebilir)
             if (varNode.initializer) {
                 DppType* initializerType = checkType(varNode.initializer, scopeManager.getCurrentScope());
                 if (initializerType.kind != DppTypeKind.Unknown && !variableType.isAssignableTo(initializerType)) { // isAssignableTo kullan
                     stderr.writeln("Hata: Satır ", varNode.name.identifierToken.lineNumber, ": Başlangıç değeri tipi ('", initializerType.name, "') değişken tipiyle ('", variableType.name, "') uyumsuz.");
                      error_reporting.reportError(...);
                     errors++;
                 }
             }
        }
    }


    return errors; // Toplam hata sayısını döndür
}

// İsteğe bağlı: Bir TypeNode AST düğümünden DppType nesnesine çeviren yardımcı fonksiyon
// Genellikle types.d veya type_system.d'de olabilir.
 DppType* getTypeFromTypeNode(TypeNode* typeNode) { ... }

// Değişken kullanımı kontrolü (semantic_analyzer'da IdentifierExpressionNode işlenirken yapılır)
// Bir IdentifierExpressionNode ile karşılaşıldığında Symbol'ün bulunup bulunmadığı kontrol edilir.
// Symbol'ün mutable olup olmadığı kontrol edilerek atama gibi işlemlerin geçerliliği denetlenir.
 int checkVariableUsage(IdentifierExpressionNode* identNode, ScopeManager* scopeManager) { ... }