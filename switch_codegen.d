module switch_codegen;

import std.stdio;
import ast;           // AST düğüm yapıları (SwitchStatementNode, BlockStatementNode, LiteralExpressionNode vb.)
import syntax_kind;   // SyntaxKind enum'ı
import ir;            // Ara Temsil yapıları (IrOpCode, IrInstruction, IrBlock)
import type_checker;  // Switch ifadesinin tipini kontrol etmek için
import scope_manager; // Gerekirse kapsam bilgisi için
import expression_codegen; // İfade kod üretimi için

// Bir switch ifadesi için IR kodu üreten fonksiyon
// node: İşlenecek SwitchStatementNode
// currentIrBlock: Şu anda IR komutlarının ekleneceği IR bloğu
// irProgram: Genel IR program yapısı (yeni bloklar eklemek için)
// scopeManager: Kapsam bilgisi (gerekirse)
// Switch ifadesinden sonraki IR bloğunu döndürür.
IrBlock* generateSwitchCode(SwitchStatementNode* node, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
    if (!node || !node.expression || !currentIrBlock || !irProgram) {
        // Hata: Geçersiz düğüm veya context
        stderr.writeln("Hata (SwitchCodegen): Geçersiz switch kod üretimi düğümü veya context.");
        // error_reporting.reportError(...);
        return currentIrBlock; // Hata durumunda mevcut bloğu döndür
    }

    writeln("Switch ifadesi kodu üretiliyor...");

    // 1. Switch ifadesi için kod üret
    // İfadenin sonucunu bir sanal kayda (Register) koyacak (Varsayım: generateExpressionCode)
    Register switchValueReg = generateExpressionCode(node.expression, currentIrBlock, irProgram, scopeManager); // Switch ifadesinin değerini hesapla


    // 2. Durum (Case) blokları, varsayılan (default) blok ve switch sonu blokları için etiketleri oluştur
    // Switch sonu bloğu ID'si
    int endOfSwitchBlockId = irProgram.blocks.length + node.cases.length + (node.defaultCase ? 1 : 0);

    // Her durum ve varsayılan durum için blok ID'lerini belirle
    int[] caseBlockIds;
    foreach (i, caseNode; node.cases) {
        caseBlockIds ~= irProgram.blocks.length + i;
    }
    int defaultBlockId = node.defaultCase ? irProgram.blocks.length + node.cases.length : -1;


    // Gerekli tüm bloklar için IR programında yer ayır
    irProgram.blocks.length = endOfSwitchBlockId + 1;


    // 3. Her durum (Case) için karşılaştırma ve koşullu atlama komutlarını üret
    foreach (i, caseNode; node.cases) {
        // Case etiketi için kod üret (genellikle sabit bir değer olmalı)
        // Burada case etiketinin sabit bir literal olduğunu varsayalım.
        // Case etiketi bir ifadeyse, o ifadenin değerini hesaplamanız gerekir.
        if (!caseNode.label || caseNode.label.kind != SyntaxKind.LiteralExpressionNode) {
            stderr.writeln("Uyarı (SwitchCodegen): Switch durumu etiketi sadece sabit literal olmalı (şimdilik).");
             error_reporting.reportWarning(...);
            // Hata kurtarma veya atlama yapılabilir
            continue; // Bu durumu atla
        }

        auto literalLabelNode = cast(LiteralExpressionNode*)caseNode.label;
        // Literal değerini IR'da temsil edilecek şekilde al (örneğin integer değeri)
        // Bu, LiteralExpressionNode'un parse edilmiş değeri olmalıdır.
        // long labelValue = literalLabelNode.parsedValue.intValue; // Varsayım

        // Basitlik için, literal değerinin token değerini kullanalım (tam sayı stringi)
         long labelValue;
         if (literalLabelNode.literalToken.kind == SyntaxKind.IntegerLiteralToken) {
            try {
                 labelValue = literalLabelNode.literalToken.value.to!long;
             } catch (Exception e) {
                  stderr.writeln("Hata: Switch durumu etiketi geçerli bir tamsayı değil.");
                   error_reporting.reportError(...);
                  continue;
             }
         } else {
             stderr.writeln("Uyarı: Switch durumu etiketi sadece tamsayı literal olmalı (şimdilik).");
             continue;
         }


        // Switch ifadesinin değeri ile case etiketi değerini karşılaştır
        // cmp switchValueReg, labelValue (Hedef assembly'de)
        // IR'da karşılaştırma komutu ve sonucu tutacak yeni bir kayıt
        Register comparisonResultReg = irProgram.nextRegister(); // Yeni bir sanal kayıt al (varsayım)
        currentIrBlock.instructions ~= {
            opCode: IrOpCode.CompareEquals, // Eşitlik karşılaştırması (Varsayım)
            dest: comparisonResultReg,
            src1: switchValueReg,
            constant: { intValue: labelValue } // Sabit değer olarak etiket değeri
        };

        // Eğer karşılaştırma sonucu true ise ilgili durum bloğuna atla
        currentIrBlock.instructions ~= {
            opCode: IrOpCode.BranchIfTrue, // Eğer karşılaştırma sonucu true ise
            src1: comparisonResultReg,      // Karşılaştırma sonucu olan kayıt
            labelId: caseBlockIds[i]       // İlgili durum bloğu ID'si
        };

        // Eğer karşılaştırma sonucu false ise, bir sonraki duruma (veya varsayılan/sona) geçmek için
        // otomatik olarak sonraki komut çalışır. Bu, kontrol akışını bir sonraki karşılaştırmaya veya atlamaya yönlendirir.
    }

    // 4. Hiçbir durum eşleşmezse gidilecek yere atlama
    // Eğer varsayılan durum varsa, varsayılan duruma atla. Yoksa switch sonuna atla.
    int jumpIfNoMatchTargetId = node.defaultCase ? defaultBlockId : endOfSwitchBlockId;
    currentIrBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: jumpIfNoMatchTargetId };


    // 5. Durum (Case) blokları için IR komutlarını üret
    foreach (i, caseNode; node.cases) {
        IrBlock* caseBlock = &irProgram.blocks[caseBlockIds[i]];
        caseBlock.id = caseBlockIds[i]; // ID'yi set et
        // Durum bloğu içeriğini üret
          statement_codegen.generateStatementCode(caseNode.body, caseBlock, irProgram, scopeManager); // Durum bloğu gövdesini üret


        // Durum bloğunun sonundan switch sonu bloğuna koşulsuz atlama ekle (eğer blok sonlandırmıyorsa)
        // Eğer blok return, break gibi bir statement ile sonlanıyorsa atlama gerekmez.
        // Switch içinde "break" statement'ı switch sonuna atlamayı sağlar.
        // Bu analiz kontrol akışı analizinde yapılır.
         if (!doesStatementTerminateExecution(caseNode.body)) { // Varsayım: doesStatementTerminateExecution fonksiyonu var
             caseBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: endOfSwitchBlockId };
              writeln("Kontrol Akışı: Durum bloğu sonrası switch sonuna atlama eklendi.");
         }
    }

    // 6. Varsayılan (Default) blok için IR komutlarını üret (eğer varsayılan durum varsa)
    if (node.defaultCase) {
        IrBlock* defaultBlock = &irProgram.blocks[defaultBlockId];
        defaultBlock.id = defaultBlockId; // ID'yi set et
        // Varsayılan blok içeriğini üret
          statement_codegen.generateStatementCode(node.defaultCase.body, defaultBlock, irProgram, scopeManager); // Varsayılan blok gövdesini üret

        // Varsayılan bloğun sonundan switch sonu bloğuna koşulsuz atlama ekle (eğer blok sonlandırmıyorsa)
         if (!doesStatementTerminateExecution(node.defaultCase.body)) {
             defaultBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: endOfSwitchBlockId };
              writeln("Kontrol Akışı: Varsayılan blok sonrası switch sonuna atlama eklendi.");
         }
    }

    // 7. Switch yapısından sonraki kod için blok (Switch sonu)
    IrBlock* endBlock = &irProgram.blocks[endOfSwitchBlockId];
    endBlock.id = endOfSwitchBlockId; // ID'yi set et
    writeln("Switch sonu bloğu oluşturuldu. ID: ", endOfSwitchBlockId);

    // Kod üretim fonksiyonu, genellikle yeni akışın devam ettiği bloğu döndürür.
    return endBlock;
}

// doesStatementTerminateExecution fonksiyonu control_flow_analyzer.d'den gelebilir.
 bool doesStatementTerminateExecution(ASTNode* node);

// irProgram.nextRegister() sanal kayıt ID'leri atamak için bir yardımcı fonksiyon olabilir.
 generateExpressionCode ve generateStatementCode fonksiyonları da expression_codegen.d ve statement_codegen.d'den gelir.