module loop_codegen;

import std.stdio;
import ast;           // AST düğüm yapıları (WhileStatementNode, ForStatementNode, DoWhileStatementNode, BlockStatementNode vb.)
import syntax_kind;   // SyntaxKind enum'ı
import ir;            // Ara Temsil yapıları (IrOpCode, IrInstruction, IrBlock)
import type_checker;  // Döngü koşulu tipini kontrol etmek için
import scope_manager; // Gerekirse kapsam bilgisi için
import expression_codegen; // İfade kod üretimi için
import statement_codegen;  // Statement kod üretimi için

// Bir while döngüsü için IR kodu üreten fonksiyon
// node: İşlenecek WhileStatementNode
// currentIrBlock: Şu anda IR komutlarının ekleneceği IR bloğu
// irProgram: Genel IR program yapısı (yeni bloklar eklemek için)
// scopeManager: Kapsam bilgisi (gerekirse)
// Döngüden sonraki IR bloğunu döndürür.
IrBlock* generateWhileCode(WhileStatementNode* node, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
    if (!node || !node.condition || !node.body || !currentIrBlock || !irProgram) {
        // Hata: Geçersiz düğüm veya context
        stderr.writeln("Hata (LoopCodegen): Geçersiz while döngüsü kod üretimi düğümü veya context.");
         error_reporting.reportError(...);
        return currentIrBlock;
    }

    writeln("While döngüsü kodu üretiliyor...");

    // Döngü yapısı için etiketleri oluştur:
    // loop_condition:  (koşulun kontrol edildiği yer)
    // loop_body:       (döngü gövdesinin başlangıcı)
    // end_of_loop:     (döngünün sonu)

    int conditionBlockId = irProgram.blocks.length; // Koşul kontrol bloğu
    int bodyBlockId = conditionBlockId + 1;      // Döngü gövdesi bloğu
    int endOfLoopBlockId = bodyBlockId + 1;      // Döngü sonu bloğu

    // Gerekli bloklar için yer ayır
    irProgram.blocks.length = endOfLoopBlockId + 1;

    // 1. Mevcut bloktan koşul kontrol bloğuna atla
    currentIrBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: conditionBlockId };

    // 2. Koşul kontrol bloğu
    IrBlock* conditionBlock = &irProgram.blocks[conditionBlockId];
    conditionBlock.id = conditionBlockId; // ID'yi set et
    writeln("Döngü koşul bloğu oluşturuldu. ID: ", conditionBlockId);

    // Koşul ifadesi için kod üret
    Register conditionResultReg = generateExpressionCode(node.condition, conditionBlock, irProgram, scopeManager); // Koşul değerini hesapla

    // Koşul true ise döngü gövdesine atla, değilse döngü sonuna atla
    conditionBlock.instructions ~= {
        opCode: IrOpCode.BranchIfTrue, // Koşul true ise
        src1: conditionResultReg,
        labelId: bodyBlockId           // Gövde bloğu ID'si
    };
    conditionBlock.instructions ~= {
        opCode: IrOpCode.Jump,         // Koşul false ise
        labelId: endOfLoopBlockId      // Döngü sonu bloğu ID'si
    };

    // 3. Döngü gövdesi bloğu
    IrBlock* bodyBlock = &irProgram.blocks[bodyBlockId];
    bodyBlock.id = bodyBlockId; // ID'yi set et
    writeln("Döngü gövde bloğu oluşturuldu. ID: ", bodyBlockId);

    // Döngü gövdesi statement'ları için kod üret
    // generateStatementCode(node.body, bodyBlock, irProgram, scopeManager); // Gövdeyi üret

    // Döngü gövdesinin sonundan koşul kontrol bloğuna geri atla
    // Eğer gövde return, break, continue gibi bir statement ile sonlanmıyorsa atlama ekle.
    // Break ve continue statement'larının kendileri uygun etiketlere atlama üretecektir.
    // Bu analiz kontrol akışı analizinde yapılabilir.
     if (!doesStatementTerminateExecution(node.body)) { // Varsayım: doesStatementTerminateExecution fonksiyonu var
         bodyBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: conditionBlockId };
          writeln("Kontrol Akışı: Döngü gövdesi sonrası koşula atlama eklendi.");
     }


    // 4. Döngü sonu bloğu
    IrBlock* endBlock = &irProgram.blocks[endOfLoopBlockId];
    endBlock.id = endOfLoopBlockId; // ID'yi set et
    writeln("Döngü sonu bloğu oluşturuldu. ID: ", endOfLoopBlockId);

    // Kod üretim fonksiyonu, genellikle yeni akışın devam ettiği bloğu döndürür.
    return endBlock;
}

// For döngüsü için IR kodu üreten fonksiyon (Çok benzer yapı)
// For döngüsünün ek olarak bir başlatma (initializer) ve bir artırma (increment/update) kısmı vardır.

IrBlock* generateForCode(ForStatementNode* node, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
    if (!node || !node.body || !currentIrBlock || !irProgram) { ... }

    writeln("For döngüsü kodu üretiliyor...");

    // Döngü yapısı için etiketleri oluştur:
    // loop_start:      (başlatma sonrası)
    // loop_condition:  (koşulun kontrol edildiği yer)
    // loop_body:       (döngü gövdesinin başlangıcı)
    // loop_update:     (artırma/güncelleme)
    // end_of_loop:     (döngünün sonu)

    int startBlockId = currentIrBlock.id; // Başlangıç bloğu mevcut blok olabilir
    int conditionBlockId = irProgram.blocks.length;
    int bodyBlockId = conditionBlockId + 1;
    int updateBlockId = bodyBlockId + 1;
    int endOfLoopBlockId = updateBlockId + 1;

    // Gerekli bloklar için yer ayır
    irProgram.blocks.length = endOfLoopBlockId + 1;

    // 1. Başlatma (Initializer) için kod üret
     generateStatementCode(node.initializer, currentIrBlock, irProgram, scopeManager); // Başlatma ifadesini üret

    // Mevcut bloktan koşul kontrol bloğuna atla
    currentIrBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: conditionBlockId };


    // 2. Koşul kontrol bloğu (generateWhileCode ile benzer)
    IrBlock* conditionBlock = &irProgram.blocks[conditionBlockId];
    conditionBlock.id = conditionBlockId;
     generateExpressionCode(node.condition, conditionBlock, irProgram, scopeManager); // Koşul değerini hesapla
    // BranchIfTrue bodyBlockId, Jump endOfLoopBlockId komutlarını ekle

    // 3. Döngü gövdesi bloğu
    IrBlock* bodyBlock = &irProgram.blocks[bodyBlockId];
    bodyBlock.id = bodyBlockId;
     generateStatementCode(node.body, bodyBlock, irProgram, scopeManager); // Gövdeyi üret

    // Döngü gövdesinin sonundan artırma/güncelleme bloğuna atla
    // Eğer gövde sonlanmıyorsa atlama ekle. Break continue durumlarını ele al.
     if (!doesStatementTerminateExecution(node.body)) {
         bodyBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: updateBlockId };
     }


    // 4. Artırma/Güncelleme bloğu
    IrBlock* updateBlock = &irProgram.blocks[updateBlockId];
    updateBlock.id = updateBlockId;
     generateStatementCode(node.update, updateBlock, irProgram, scopeManager); // Artırma/güncelleme ifadesini üret

    // Artırma/güncelleme bloğunun sonundan koşul kontrol bloğuna geri atla
    updateBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: conditionBlockId };


    // 5. Döngü sonu bloğu
    IrBlock* endBlock = &irProgram.blocks[endOfLoopBlockId];
    endBlock.id = endOfLoopBlockId;

    return endBlock;
}
*/

// Do-While döngüsü için IR kodu üreten fonksiyon (Koşul sonda kontrol edilir)

IrBlock* generateDoWhileCode(DoWhileStatementNode* node, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
    if (!node || !node.condition || !node.body || !currentIrBlock || !irProgram) { ... }

    writeln("Do-While döngüsü kodu üretiliyor...");

    // Döngü yapısı için etiketleri oluştur:
    // loop_body:       (döngü gövdesinin başlangıcı)
    // loop_condition:  (koşulun kontrol edildiği yer - gövdeden sonra)
    // end_of_loop:     (döngünün sonu)

    int bodyBlockId = irProgram.blocks.length; // Gövde bloğu (başlangıç)
    int conditionBlockId = bodyBlockId + 1;   // Koşul kontrol bloğu
    int endOfLoopBlockId = conditionBlockId + 1; // Döngü sonu bloğu

    // Gerekli bloklar için yer ayır
    irProgram.blocks.length = endOfLoopBlockId + 1;

    // 1. Mevcut bloktan döngü gövdesi bloğuna atla (Do-while en az bir kere çalışır)
    currentIrBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: bodyBlockId };

    // 2. Döngü gövdesi bloğu
    IrBlock* bodyBlock = &irProgram.blocks[bodyBlockId];
    bodyBlock.id = bodyBlockId;
     generateStatementCode(node.body, bodyBlock, irProgram, scopeManager); // Gövdeyi üret

    // Döngü gövdesinin sonundan koşul kontrol bloğuna atla
    // Eğer gövde sonlanmıyorsa atlama ekle. Break continue durumlarını ele al.
     if (!doesStatementTerminateExecution(node.body)) {
         bodyBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: conditionBlockId };
     }


    // 3. Koşul kontrol bloğu
    IrBlock* conditionBlock = &irProgram.blocks[conditionBlockId];
    conditionBlock.id = conditionBlockId;
     generateExpressionCode(node.condition, conditionBlock, irProgram, scopeManager); // Koşul değerini hesapla

    // Koşul true ise döngü gövdesinin BAŞINA geri atla, değilse döngü sonuna atla
    conditionBlock.instructions ~= {
        opCode: IrOpCode.BranchIfTrue, // Koşul true ise
        src1: conditionResultReg, // Koşul sonucu olan kayıt
        labelId: bodyBlockId           // Gövde bloğu ID'si (başa dön)
    };
    conditionBlock.instructions ~= {
        opCode: IrOpCode.Jump,         // Koşul false ise
        labelId: endOfLoopBlockId      // Döngü sonu bloğu ID'si
    };

    // 4. Döngü sonu bloğu
    IrBlock* endBlock = &irProgram.blocks[endOfLoopBlockId];
    endBlock.id = endOfLoopBlockId;

    return endBlock;
}


// doesStatementTerminateExecution fonksiyonu control_flow_analyzer.d'den gelebilir.
// generateExpressionCode ve generateStatementCode fonksiyonları expression_codegen.d ve statement_codegen.d'den gelir.