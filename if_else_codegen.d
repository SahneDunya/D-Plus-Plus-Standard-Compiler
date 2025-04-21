module if_else_codegen;

import std.stdio;
import ast;           // AST düğüm yapıları (IfStatementNode, BlockStatementNode vb.)
import syntax_kind;   // SyntaxKind enum'ı
import ir;            // Ara Temsil yapıları (IrOpCode, IrInstruction, IrBlock)
import type_checker;  // Koşul tipini kontrol etmek için
import scope_manager; // Gerekirse kapsam bilgisi için
import expression_codegen; // İfade kod üretimi için (aşağıda varsayılacak)

// Bir if-else ifadesi için IR kodu üreten fonksiyon
// node: İşlenecek IfStatementNode
// currentIrBlock: Şu anda IR komutlarının ekleneceği IR bloğu
// irProgram: Genel IR program yapısı (yeni bloklar eklemek için)
// scopeManager: Kapsam bilgisi (gerekirse)
// Belirlenen (conditional jump sonucu) hedef IR bloğunu döndürür veya null.
IrBlock* generateIfElseCode(IfStatementNode* node, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
    if (!node || !node.condition || !node.thenBlock || !currentIrBlock || !irProgram) {
        // Hata: Geçersiz düğüm veya context
        stderr.writeln("Hata (IfElseCodegen): Geçersiz if-else kod üretimi düğümü veya context.");
        // error_reporting.reportError(...);
        return currentIrBlock; // Hata durumunda mevcut bloğu döndür
    }

    writeln("If-Else kodu üretiliyor...");

    // 1. Koşul ifadesi için kod üret
    // Koşul ifadesinin sonucu bir boolean değer olmalı (TypeChecker tarafından kontrol edildi)
    // expression_codegen.generateExpressionCode fonksiyonu koşul için IR komutları üretecek
    // ve sonucunu bir sanal kayda (Register) koyacak (Varsayım).
    Register conditionResultReg = generateExpressionCode(node.condition, currentIrBlock, irProgram, scopeManager); // Varsayım: Expression codegen fonksiyonu var


    // 2. Dallanma etiketlerini oluştur
    // Eğer koşul true ise gidilecek blok (then bloğu)
    // Eğer else dalı varsa, koşul false ise gidilecek blok (else bloğu)
    // If-else yapısından sonra gidilecek blok (yapının sonu)
    int thenBlockId = irProgram.blocks.length; // Yeni then bloğu ID'si
    int elseBlockId = -1; // Else bloğu ID'si (başlangıçta -1)
    int endOfIfElseBlockId = thenBlockId + 1; // If-else sonu bloğu ID'si (else yoksa)

    if (node.elseBranch) {
        elseBlockId = thenBlockId + 1; // Yeni else bloğu ID'si
        endOfIfElseBlockId = elseBlockId + 1; // If-else sonu bloğu ID'si (else varsa)
    }

    // Yeni blokları IR programına ekle (henüz içlerini doldurmadık)
    irProgram.blocks.length = endOfIfElseBlockId + 1; // Gerekli tüm bloklar için yer ayır

    // 3. Koşullu atlama komutunu üret (Eğer koşul false ise nereye gidilecek?)
    // Eğer else dalı varsa, koşul false ise else bloğuna atla.
    // Eğer else dalı yoksa, koşul false ise if-else sonu bloğuna atla.
    int jumpIfFalseTargetId = node.elseBranch ? elseBlockId : endOfIfElseBlockId;
    currentIrBlock.instructions ~= {
        opCode: IrOpCode.BranchIfTrue, // Koşul true ise devam et, değilse atla (veya tam tersi)
        src1: conditionResultReg,      // Koşul sonucu olan kayıt
        labelId: jumpIfFalseTargetId   // Koşul false ise atlanacak etiket (else veya if-else sonu)
        // BranchIfTrue true ise sonraki komuta (then bloğuna giden atlama veya ilk komut),
        // false ise labelId'ye atlar varsayalım.
    };
     // Veya:
      currentIrBlock.instructions ~= { opCode: IrOpCode.BranchIfFalse, src1: conditionResultReg, labelId: jumpIfFalseTargetId };


    // 4. Then bloğu için IR komutlarını üret
    // Yeni bir IR bloğu oluştur veya mevcut olanı kullan
    IrBlock* thenBlock = &irProgram.blocks[thenBlockId];
    thenBlock.id = thenBlockId; // ID'yi set et
    // Then bloğu içeriğini üret
    // statement_codegen.generateStatementCode fonksiyonu then bloğundaki statementlar için kod üretecek.
     // Bu fonksiyon BlockStatementNode'u işleyebilir.
      generateStatementCode(node.thenBlock, thenBlock, irProgram, scopeManager); // Varsayım: Statement codegen fonksiyonu var


    // Then bloğunun sonundan if-else sonu bloğuna koşulsuz atlama ekle (eğer then bloğu sonlandırmıyorsa)
    // Eğer then bloğu return gibi bir statement ile sonlanıyorsa atlama gerekmez.
     if (!doesStatementTerminateExecution(node.thenBlock)) { // Kontrol akışı analizinden gelen bilgi kullanılabilir
         thenBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: endOfIfElseBlockId };
         writeln("Kontrol Akışı: Then bloğu sonrası if-else sonuna atlama eklendi.");
     }


    // 5. Else bloğu için IR komutlarını üret (eğer else dalı varsa)
    if (node.elseBranch) {
        IrBlock* elseBlock = &irProgram.blocks[elseBlockId];
        elseBlock.id = elseBlockId; // ID'yi set et
        // Else bloğu içeriğini üret
          generateStatementCode(node.elseBranch, elseBlock, irProgram, scopeManager); // Varsayım: Statement codegen fonksiyonu var

        // Else bloğunun sonundan if-else sonu bloğuna koşulsuz atlama ekle (eğer else bloğu sonlandırmıyorsa)
         if (!doesStatementTerminateExecution(node.elseBranch)) {
             elseBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: endOfIfElseBlockId };
              writeln("Kontrol Akışı: Else bloğu sonrası if-else sonuna atlama eklendi.");
         }
    }


    // 6. If-else yapısından sonraki kod için blok (If-else sonu)
    // Bu blok zaten yukarıda yer ayrıldı. Sadece ID'sini set edelim ve kullanılabilir olduğunu işaretleyelim.
    IrBlock* endBlock = &irProgram.blocks[endOfIfElseBlockId];
    endBlock.id = endOfIfElseBlockId; // ID'yi set et
    // Buraya gelen komutlar, then veya else dallarından gelen atlamaların hedefi olacaktır.
    writeln("If-Else sonu bloğu oluşturuldu. ID: ", endOfIfElseBlockId);


    // Kod üretim fonksiyonu, genellikle yeni akışın devam ettiği bloğu döndürür.
    // If-else durumunda, bu if-else sonu bloğudur.
    return endBlock;
}

// Yardımcı fonksiyon: Bir ifadenin kodunu üreten (Çok basitleştirilmiş varsayım)
// Gerçekte expression_codegen.d dosyasında yer alacaktır.
// IR komutlarını currentIrBlock'a ekler ve sonuç kaydını döndürür.
Register generateExpressionCode(ASTNode* expressionNode, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
    stderr.writeln("Uyarı: generateExpressionCode fonksiyonu implemente edilmedi. Placeholder kod üretiliyor.");
    // Basitlik için sabit bir değer yükleyen komut üretelim
    currentIrBlock.instructions ~= { opCode: IrOpCode.LoadConstant, dest: 1, constant: {intValue: 1, stringValue: "1"} }; // Kayıt 1'e 1 yükle
    return 1; // Sonuç kaydı 1 olsun
}

// Yardımcı fonksiyon: Bir statement'ın kodunu üreten (Çok basitleştirilmiş varsayım)
// Genellikle statement_codegen.d dosyasında yer alacaktır.
// IR komutlarını currentIrBlock'a ekler.
// Bir blok statement'ı alırsa içindeki tüm statement'ları dolaşır.
void generateStatementCode(ASTNode* statementNode, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
     stderr.writeln("Uyarı: generateStatementCode fonksiyonu implemente edilmedi. Placeholder kod üretiliyor.");
    if (!statementNode) return;

    // Eğer bir blok statement ise, içindeki her statement için recursive çağrı yap
    if (statementNode.kind == SyntaxKind.BlockStatementNode) {
        foreach (stmt; statementNode.children) {
            generateStatementCode(stmt, currentIrBlock, irProgram, scopeManager);
        }
        return; // Bloğu işledik
    }

    // Diğer statement türleri (return, variable declaration, expression statement vb.) için kod üretimi burada olur.
    // Örneğin:
     if (statementNode.kind == SyntaxKind.ReturnStatementNode) { ... }
     if (statementNode.kind == SyntaxKind.ExpressionStatementNode) {
         auto exprStmt = cast(ExpressionStatementNode*)statementNode;
         generateExpressionCode(exprStmt.expression, currentIrBlock, irProgram, scopeManager); // İfade kodunu üret
     }

    // Bilinmeyen statement türü için Nop ekleyelim
    currentIrBlock.instructions ~= { opCode: IrOpCode.Nop };
}