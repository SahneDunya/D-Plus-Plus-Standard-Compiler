module function_call;

import std.stdio;
import std.array;
import ast;           // AST düğüm yapıları (CallExpressionNode, IdentifierExpressionNode vb.)
import syntax_kind;   // SyntaxKind enum'ı
import dpp_types;     // D++ tür temsilcileri (FunctionType gibi)
import symbol_table;  // Symbol yapısı
import scope_manager; // Sembol çözümlemesi için
import type_system;   // Tür sistemi kuralları (fonksiyon imza uyumluluğu)
import ir;            // Ara Temsil yapıları (IrOpCode, IrInstruction, IrBlock)
import expression_codegen; // Argüman ifadeleri için kod üretimi
import calling_convention; // Çağırma kuralları için

// Bir fonksiyon çağrısı ifadesini anlamsal olarak analiz eden ve IR kodu üreten fonksiyon
// Genellikle semantic_analyzer'daki AST traversi sırasında CallExpressionNode ile karşılaşınca çağrılır.
// İfadenin (fonksiyon çağrısının) belirlenen tipini döndürür ve hata varsa raporlar.
// node: İşlenecek CallExpressionNode
// currentIrBlock: Şu anda IR komutlarının ekleneceği IR bloğu
// irProgram: Genel IR program yapısı (yeni bloklar eklemek için)
// scopeManager: Kapsam bilgisi için
// callingConv: Kullanılacak çağırma kuralı
DppType* analyzeAndGenerateCallCode(CallExpressionNode* node, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager, CallingConvention* callingConv) {
    if (!node || !node.function || !currentIrBlock || !irProgram || !scopeManager || !callingConv) {
        // Hata: Geçersiz düğüm veya context
        stderr.writeln("Hata (FunctionCall): Geçersiz fonksiyon çağrısı kod üretimi düğümü veya context.");
         error_reporting.reportError(...);
        return DppType.Unknown;
    }

    writeln("Fonksiyon çağrısı analizi ve kodu üretiliyor...");

    int errors = 0;

    // 1. Çağrılan fonksiyonu çözümle (Semantik analizci tarafından yapılmış olmalı, ama burada teyit edelim)
    // Çağrılan 'function' düğümü genellikle bir IdentifierExpressionNode olacaktır.
    // Eğer bir üye erişimi (obj.method()) ise daha karmaşık çözünürlük gerekir.
    if (node.function.kind != SyntaxKind.IdentifierExpressionNode) {
         stderr.writeln("Uyarı (FunctionCall): Şimdilik sadece tanımlayıcı ile fonksiyon çağrısı destekleniyor.");
         error_reporting.reportWarning(...);
        // Hata kurtarma veya atlama
         return DppType.Unknown;
    }
    auto functionIdentNode = cast(IdentifierExpressionNode*)node.function;
    Symbol* functionSymbol = scopeManager.resolveSymbol(functionIdentNode.value); // Fonksiyon sembolünü çöz

    if (!functionSymbol || functionSymbol.kind != SyntaxKind.FnKeyword) { // Sembol bulunamadı veya fonksiyon değil
        stderr.writeln("Hata: Satır ", functionIdentNode.identifierToken.lineNumber, ", Sütun ", functionIdentNode.identifierToken.columnNumber, ": Fonksiyon '", functionIdentNode.value, "' bulunamadı veya çağrılamaz.");
         error_reporting.reportError(...);
        errors++;
        return DppType.Unknown;
    }

    // Çağrılan sembolün tipinin gerçekten bir FunctionType olup olmadığını kontrol et
    if (functionSymbol.type.kind != DppTypeKind.Function) {
         stderr.writeln("Hata: Satır ", functionIdentNode.identifierToken.lineNumber, ": '", functionSymbol.name, "' çağrılamaz bir tipe ('", functionSymbol.type.name, "') sahip.");
          error_reporting.reportError(...);
         errors++;
         return DppType.Unknown;
    }
    auto calledFunctionType = cast(FunctionType*)functionSymbol.type; // FunctionType'a cast et

    // 2. Argüman tiplerini belirle ve fonksiyon imza uyumluluğunu kontrol et
    // Her argüman ifadesi için kod üret ve tipini belirle.
    Register[] argumentValueRegs; // Argüman değerlerinin konulacağı kayıtlar
    DppType*[] argumentTypes;    // Argümanların belirlenen tipleri

    foreach (argNode; node.arguments) {
        // Argüman ifadesi için kod üret
        Register argReg = generateExpressionCode(argNode, currentIrBlock, irProgram, scopeManager); // Argüman ifadesinin değerini hesapla
        argumentValueRegs ~= argReg;

        // Argüman ifadesinin tipini belirle
         DppType* argType = checkType(argNode, scopeManager.getCurrentScope()); // type_checker.d'deki fonksiyon
        argumentTypes ~= argType;
    }

    // Argüman sayısı ve tiplerinin fonksiyon imza uyumluğunu kontrol et (TypeSystem)
    // Bu mantık type_system.d'de ayrı bir fonksiyonda olabilir.
     bool isSignatureMatch(FunctionType* calledType, DppType*[] argTypes);
     if (calledFunctionType.parameterTypes.length != argumentTypes.length) {
         stderr.writeln("Hata: Satır ", node.function.kind.to!string, ": Fonksiyon '", functionSymbol.name, "' beklenen argüman sayısı ", calledFunctionType.parameterTypes.length, ", ancak ", argumentTypes.length, " argüman verildi.");
          error_reporting.reportError(...);
         errors++;
     } else {
         // Argüman tiplerini parametre tipleriyle karşılaştır
         for (size_t i = 0; i < argumentTypes.length; ++i) {
            // isAssignableTo veya areTypesEqual kullanılarak uyumluluk kontrol edilir.
             if (!argumentTypes[i].isAssignableTo(calledFunctionType.parameterTypes[i])) {
                 stderr.writeln("Hata: Satır ", node.function.kind.to!string, ": Fonksiyon '", functionSymbol.name, "' için ", i + 1, ". argüman tipi ('", argumentTypes[i].name, "') parametre tipiyle ('", calledFunctionType.parameterTypes[i].name, "') uyumsuz.");
                  error_reporting.reportError(...);
                 errors++;
             }
         }
     }

    // Eğer anlamsal hatalar varsa kod üretimine devam etme
    if (errors > 0) {
        return DppType.Unknown;
    }


    // 3. Çağırma kuralına göre argümanları hazırla ve fonksiyonu çağır
    // Çağırma kuralı nesnesi argümanların nasıl geçirileceğini belirler.
    // Bu kısım oldukça karmaşıktır ve çağırma kuralının detaylarına bağlıdır.
    // Argümanların kayıtlara veya stack'e kopyalanması/taşınması (move semantics dikkate alınarak).
    writeln("Çağırma Kuralı kullanarak argümanlar hazırlanıyor...");
    // callingConv.determineArgumentPassing(argumentTypes); // Bilgiyi al

    // Örnek: Argümanları çağırma kuralına göre uygun kayıtlara kopyalayan/taşıyan IR komutları üret
    // Örneğin (varsayımsal SystemV AMD64 için ilk iki argüman):
     currentIrBlock.instructions ~= { opCode: IrOpCode.CopyRegister, dest: RAX_Reg, src1: argumentValueRegs[0] }; // RDI yerine RAX kullanalım basitlik için
     currentIrBlock.instructions ~= { opCode: IrOpCode.CopyRegister, dest: RBX_Reg, src1: argumentValueRegs[1] }; // RSI yerine RBX kullanalım

    // IR seviyesinde, Call komutu genellikle doğrudan argüman register'larını veya stack bilgilerini belirtir.
    // Veya daha yüksek seviyeli bir Call komutu alıp backend'in çağırma kuralına göre detaylandırmasını beklersiniz.

    // 4. Fonksiyon çağrısı IR komutunu üret
    // Bu komut, çağrılacak fonksiyonu (sembolünü veya adresini) ve argüman bilgilerini içerir.
    // Eğer fonksiyon değer döndürüyorsa, dönüş değeri için bir hedef kayıt belirtilmelidir.
    Register resultReg = irProgram.nextRegister(); // Fonksiyon dönüş değeri için kayıt
    if (calledFunctionType.returnType == DppType.VoidType) {
        resultReg = IrProgram.NoResultReg; // Void fonksiyonlar değer döndürmez (Varsayım)
    }


    currentIrBlock.instructions ~= {
        opCode: IrOpCode.Call,
        dest: resultReg,           // Dönüş değeri konulacak kayıt (void değilse)
         functionInfo: functionSymbol, // Çağrılan fonksiyon sembolü (veya doğrudan adresi/ID'si)
         args: argumentValueRegs       // Argüman değerlerini içeren kayıtlar
         functionName: functionSymbol.name // Basitlik için sadece fonksiyon adını kullanalım
         // Argüman kayıtları/stack bilgisi çağırma kuralına göre detaylandırılmalıdır.
    };
     writeln("Fonksiyon çağrısı IR komutu eklendi: ", functionSymbol.name);


    // 5. Çağırma kuralına göre dönüş değerini işle
    // Eğer fonksiyon değer döndürüyorsa, bu değer çağırma kuralına göre belirli bir kayda konur.
    // generateExpressionCode fonksiyonu bu dönüş değerinin konulduğu kaydı döndürmelidir.
    // Yukarıda Call komutunun dest alanına dönüş değerinin konulacağı kayıt zaten belirtildi.
    // Burada ek bir şey yapmaya gerek yok, sadece bilginin nasıl aktarıldığını anlamak önemli.

    // Çağrı ifadesinin tipini belirle (Fonksiyonun dönüş tipi)
     node.resolvedType = calledFunctionType.returnType; // Gelişmiş AST

    return calledFunctionType.returnType; // Çağrının sonucunun tipini döndür
}

// generateExpressionCode fonksiyonu expression_codegen.d'den gelir.
// checkType fonksiyonu type_checker.d'den gelir.
// irProgram.nextRegister() ve IrProgram.NoResultReg sanal kayıt yönetimiyle ilgili helper'lardır.