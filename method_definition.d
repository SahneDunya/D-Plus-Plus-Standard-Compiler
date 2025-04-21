module method_definition;

import std.stdio;
import std.array;
import ast;           // AST düğüm yapıları (FunctionDeclarationNode, ParameterNode, TypeNode, BlockStatementNode vb.)
import syntax_kind;   // SyntaxKind enum'ı
import dpp_types;     // D++ tür temsilcileri (FunctionType gibi)
import symbol_table;  // Symbol yapısı
import scope_manager; // Kapsam yönetimi için
import type_system;   // Tür sistemi kuralları (örneğin, getTypeFromTypeNode gibi)
import ir;            // Ara Temsil yapıları (IrFunction, IrBlock)
import statement_codegen; // Fonksiyon gövdesi kod üretimi için (recursive çağrı)
import error_reporting; // Hata raporlama için


// Bir fonksiyon bildirimini işleyen ana fonksiyon
// Genellikle semantic_analyzer'daki AST traversi sırasında FunctionDeclarationNode ile karşılaşınca çağrılır.
// Fonksiyon için oluşturulan IrFunction yapısını döndürebilir veya null hata durumunda.
IrFunction* processFunctionDeclaration(FunctionDeclarationNode* node, IrProgram* irProgram, ScopeManager* scopeManager) {
    if (!node || !node.name || !irProgram || !scopeManager) {
        // Hata: Geçersiz düğüm veya context
        stderr.writeln("Hata (MethodDefinition): Geçersiz fonksiyon bildirim düğümü veya context.");
         error_reporting.reportError(...);
        return null;
    }

    writeln("Fonksiyon bildirimi işleniyor: ", node.name.value);

    string functionName = node.name.value;

    // 1. Fonksiyonun tipini belirle (Dönüş tipi ve parametre tiplerini içeren FunctionType)
    DppType* returnType = DppType.VoidType; // Varsayılan dönüş tipi void
    if (node.returnType) {
        // Dönüş tipi belirtilmişse, ilgili DppType nesnesini al
        // returnType = getTypeFromTypeNode(node.returnType); // type_system.d'deki fonksiyon
         stderr.writeln("Uyarı (MethodDefinition): Dönüş tipi çözme implemente edilmedi."); // Placeholder
         returnType = DppType.IntType; // Geçici olarak int varsayalım
    }

    DppType*[] parameterTypes;
    // Parametre tiplerini işle
    foreach (paramNode; node.parameters) {
        if (paramNode.type) {
            // Parametre tipi belirtilmişse, DppType nesnesini al
            // DppType* paramType = getTypeFromTypeNode(paramNode.type); // type_system.d'deki fonksiyon
             stderr.writeln("Uyarı (MethodDefinition): Parametre tipi çözme implemente edilmedi."); // Placeholder
             DppType* paramType = DppType.IntType; // Geçici olarak int varsayalım
             parameterTypes ~= paramType;
        } else {
            // Hata: Parametre tipi belirtilmeli (D++ tip çıkarımını fonksiyon parametrelerinde desteklemiyorsa)
            stderr.writeln("Hata: Satır ", paramNode.name.identifierToken.lineNumber, ": Fonksiyon parametresi için tip belirtilmeli.");
             error_reporting.reportError(...);
            // Hata durumunda UnknownType ekleyebilir veya işlemi durdurabilirsiniz.
             parameterTypes ~= DppType.Unknown;
        }
    }

    // Fonksiyon tipini oluştur
    DppType* functionType = new FunctionType(returnType, parameterTypes); // types.d'deki sınıf


    // 2. Fonksiyon adını sembol tablosuna ekle (mevcut kapsama - genellikle global veya sınıf kapsamı)
    // ScopeManager zaten aynı isimde sembol varsa hata verecektir.
    Symbol functionSymbol = {
        name: functionName,
        type: functionType, // Fonksiyonun tipi
        kind: SyntaxKind.FnKeyword, // Sembol türü: Fonksiyon
        isMutable: false // Fonksiyonlar genellikle mutable değildir
         declarationNode: node // İsteğe bağlı olarak AST düğümüne referans
    };

    if (!scopeManager.addSymbol(functionSymbol)) {
        // ScopeManager hata raporladıysa, burada ek bir şey yapmaya gerek yok, sadece null döndür.
        return null;
    }

    // 3. Fonksiyon için IR yapısını oluştur
    // Yeni bir IrFunction nesnesi oluştur ve IrProgram'a ekle.
    IrFunction newIrFunction;
    // newIrFunction.info = functionSymbol; // İsteğe bağlı olarak sembol bilgilerini ekle
    irProgram.functions ~= newIrFunction;
    // IrProgram'daki son eklenen fonksiyonun pointer'ını al
    IrFunction* currentIrFunction = &irProgram.functions.back;


    // 4. Fonksiyon gövdesi için yeni bir kapsam gir
    scopeManager.enterScope();

    // Fonksiyon parametrelerini yeni kapsamda sembol tablosuna ekle
    // Parametreler, fonksiyon gövdesi içinde geçerlidir.
    foreach (i, paramNode; node.parameters) {
        if (paramNode.name && i < parameterTypes.length) {
            Symbol paramSymbol = {
                name: paramNode.name.value,
                type: parameterTypes[i], // Önceden belirlenen parametre tipi
                kind: SyntaxKind.IdentifierToken, // Sembol türü: Değişken/Parametre
                isMutable: paramNode.isMutable // Parametre mutable olabilir mi?
                 declarationNode: paramNode // İsteğe bağlı
            };
             scopeManager.addSymbol(paramSymbol); // Yeni (fonksiyon gövdesi) kapsamına ekle
        }
    }

    // 5. Fonksiyon gövdesi için kod üretimine başla
    // Genellikle fonksiyon gövdesi bir BlockStatementNode'dur.
    // Gövde için bir başlangıç bloğu oluştur.
    IrBlock* entryBlock = new IrBlock();
    entryBlock.id = irProgram.nextBlockId(); // Yeni blok ID'si al (varsayım)
    currentIrFunction.blocks ~= entryBlock;

    // Fonksiyon gövdesi statement'ları için kod üret
    // generateStatementCode fonksiyonu BlockStatementNode'u işleyebilir.
    // Bu, gövde içindeki tüm statementları dolaşacak ve IR üretecektir.
      generateStatementCode(node.body, entryBlock, irProgram, scopeManager); // Gövdeyi üret


    // Fonksiyon gövdesinin sonundan dönüş komutu ekle (eğer sonlanmıyorsa ve dönüş tipi void ise)
    // Eğer dönüş tipi void değilse ve tüm yollar return statement'ı ile sonlanmıyorsa hata verilmelidir (kontrol akışı analizi).
    // Eğer void fonksiyon ve sonlanmıyorsa otomatik return eklenir.
     if (!doesStatementTerminateExecution(node.body) && returnType == DppType.VoidType) {
         // Son bloğu bul (gövde üretiminin son bloğu)
         IrBlock* lastBlockInBody = currentIrFunction.blocks.back; // Varsayım: son block
         lastBlockInBody.instructions ~= { opCode: IrOpCode.Return }; // Void return
          writeln("Kontrol Akışı: Void fonksiyon sonu, otomatik return eklendi.");
     }


    // 6. Fonksiyon gövdesinin kapsamından çık
    scopeManager.exitScope();

    writeln("Fonksiyon bildirimi işleme tamamlandı: ", functionName);

    // Fonksiyon için oluşturulan IrFunction pointer'ını döndür
    return currentIrFunction;
}

// doesStatementTerminateExecution fonksiyonu control_flow_analyzer.d'den gelebilir.
// generateStatementCode fonksiyonu statement_codegen.d'den gelir.
// getTypeFromTypeNode fonksiyonu type_system.d'den gelebilir.
// irProgram.nextBlockId() yeni IR blok ID'leri atamak için bir yardımcı fonksiyon olabilir.