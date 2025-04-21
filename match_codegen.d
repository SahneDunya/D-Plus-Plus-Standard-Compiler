module match_codegen;

import std.stdio;
import std.array;
import ast;           // AST düğüm yapıları (MatchExpressionNode, MatchArmNode, LiteralPatternNode vb.)
import syntax_kind;   // SyntaxKind enum'ı
import ir;            // Ara Temsil yapıları (IrOpCode, IrInstruction, IrBlock)
import type_checker;  // Match ifadesinin ve desenlerinin tiplerini kontrol etmek için
import scope_manager; // Kapsam ve sembol yönetimi için (desenlerdeki değişken yakalama için)
import expression_codegen; // İfade kod üretimi için

// Bir match ifadesi/desen eşleştirme için IR kodu üreten fonksiyon
// node: İşlenecek MatchExpressionNode
// currentIrBlock: Şu anda IR komutlarının ekleneceği IR bloğu
// irProgram: Genel IR program yapısı (yeni bloklar eklemek için)
// scopeManager: Kapsam bilgisi (desenlerde yeni değişkenler tanımlanabilir)
// Match ifadesinden sonraki IR bloğunu döndürür.
IrBlock* generateMatchCode(MatchExpressionNode* node, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
    if (!node || !node.expression || !node.arms.length || !currentIrBlock || !irProgram) {
        // Hata: Geçersiz düğüm veya context
        stderr.writeln("Hata (MatchCodegen): Geçersiz match kod üretimi düğümü veya context.");
         error_reporting.reportError(...);
        return currentIrBlock; // Hata durumunda mevcut bloğu döndür
    }

    writeln("Match ifadesi kodu üretiliyor...");

    // 1. Eşleştirilecek ifade için kod üret
    // İfadenin sonucunu bir sanal kayda (Register) koyacak.
    Register matchedValueReg = generateExpressionCode(node.expression, currentIrBlock, irProgram, scopeManager); // Eşleştirilecek değeri hesapla


    // 2. Her match kolu (arm) ve match sonu için blok ID'lerini oluştur
    // Match sonu bloğu ID'si
    int endOfMatchBlockId = irProgram.blocks.length + node.arms.length;

    // Her kol (arm) için blok ID'lerini belirle
    int[] armBlockIds;
    foreach (i, armNode; node.arms) {
        armBlockIds ~= irProgram.blocks.length + i;
    }


    // Gerekli tüm bloklar için IR programında yer ayır
    irProgram.blocks.length = endOfMatchBlockId + 1;


    // 3. Her match kolu (arm) için desen eşleştirme kontrolü ve dallanma komutlarını üret
    // Desenler sırayla denenir. İlk eşleşen desene ait koda atlanır.
    // Eğer bir desen eşleşmezse, sıradaki desene geçilir.
    IrBlock* currentCheckBlock = currentIrBlock; // Kontrollerin ekleneceği mevcut blok

    foreach (i, armNode; node.arms) {
        // Bu kola ait kodu içerecek blok
        IrBlock* armBlock = &irProgram.blocks[armBlockIds[i]];
        armBlock.id = armBlockIds[i]; // ID'yi set et

        // Desenin karmaşıklığına göre eşleştirme kodu üretilir.
        // Burada SADECE BASİT LİTERAL DESENLERİ ele alıyoruz.
        if (armNode.pattern.kind == SyntaxKind.LiteralPatternNode) { // Varsayım: LiteralPatternNode düğümü var
            auto literalPatternNode = cast(LiteralPatternNode*)armNode.pattern;

            // Eşleştirilecek değer ile desen literal değerini karşılaştır.
             // getBinaryOperatorResultType veya benzeri ile tiplerin uyumlu olduğu semantik analizde kontrol edildi.

             long patternValue;
             if (literalPatternNode.literalToken.kind == SyntaxKind.IntegerLiteralToken) {
                 try {
                      patternValue = literalPatternNode.literalToken.value.to!long;
                  } catch (Exception e) {
                       stderr.writeln("Hata: Match deseni etiketi geçerli bir tamsayı değil.");
                        error_reporting.reportError(...);
                       // Hata kurtarma veya atlama yapılabilir
                       continue; // Bu kolu atla
                  }
             } else {
                 stderr.writeln("Uyarı: Match deseni etiketi sadece tamsayı literal olmalı (şimdilik).");
                 continue;
             }


            // Eşleştirilen değer ile desen değeri karşılaştır
            Register comparisonResultReg = irProgram.nextRegister(); // Yeni bir sanal kayıt al
            currentCheckBlock.instructions ~= {
                opCode: IrOpCode.CompareEquals, // Eşitlik karşılaştırması
                dest: comparisonResultReg,
                src1: matchedValueReg,      // Eşleştirilen değer
                constant: { intValue: patternValue } // Desen değeri
            };

            // Eğer karşılaştırma sonucu true ise ilgili kol bloğuna atla
            currentCheckBlock.instructions ~= {
                opCode: IrOpCode.BranchIfTrue, // Eğer karşılaştırma sonucu true ise
                src1: comparisonResultReg,      // Karşılaştırma sonucu olan kayıt
                labelId: armBlockIds[i]        // İlgili kol bloğu ID'si
            };

            // Eğer eşleşmezse, bir sonraki kola geçmek için otomatik olarak sonraki komut çalışır.
            // Bir sonraki kolun kontrolü için yeni bir blok oluştur (veya son kola kadar devam et)
             if (i < node.arms.length - 1) {
                 int nextCheckBlockId = irProgram.blocks.length; // Yeni kontrol bloğu ID'si
                 irProgram.blocks.length = nextCheckBlockId + 1; // Yer ayır
                 currentCheckBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: nextCheckBlockId }; // Sıradaki kontrol bloğuna atla
                 currentCheckBlock = &irProgram.blocks[nextCheckBlockId]; // Mevcut kontrol bloğunu güncelle
                 currentCheckBlock.id = nextCheckBlockId; // ID'yi set et
             }


        } else {
             // Daha karmaşık desen türleri (değişken yakalama, struct desenleri vb.)
             // Bu kısım, desenin yapısına göre özel kod üretimi gerektirir.
              stderr.writeln("Uyarı (MatchCodegen): Daha karmaşık desen türleri implemente edilmedi: ", armNode.pattern.kind.to!string);
              // Hata kurtarma: Bu kolu atla veya hata ver.
              // currentCheckBlock'tan direkt bir sonraki kontrol bloğuna veya sona atlama eklenebilir.
        }

    }

    // 4. Hiçbir desen eşleşmezse ne yapılacağı (Match'in kapsamlılığı kontrolü semantik analizde yapılır)
    // Eğer match kapsamlı (exhaustive) ise bu noktaya gelinmemelidir.
    // Eğer kapsamlı değilse ve buraya gelinirse, genellikle bir çalışma zamanı hatası (panic) fırlatılır.
     stderr.writeln("Uyarı (MatchCodegen): Match ifadesi kapsamlı değil veya beklenmedik durum oluştu. Çalışma zamanı hatası üretilebilir.");
    // Panic/hata kodu üretimi buraya eklenebilir.
     currentCheckBlock.instructions ~= { opCode: IrOpCode.Panic }; // Varsayım: Panic komutu var


    // 5. Her match kolu (arm) bloğu için IR komutlarını üret
    foreach (i, armNode; node.arms) {
        IrBlock* armBlock = &irProgram.blocks[armBlockIds[i]];
        // Arm bloğu içeriğini üret (ifade veya statement olabilir)
        // Match bir ifadeyse, her kolun son ifadesinin değeri match ifadesinin sonucudur.
        // Bu değer match sonu bloğuna iletilmelidir (örneğin, bir kayda konularak).
          generateCodeForMatchArmBody(armNode.body, armBlock, irProgram, scopeManager); // Varsayım: Arm gövdesi kod üretimi fonksiyonu var

        // Match kolu bloğunun sonundan match sonu bloğuna koşulsuz atlama ekle
        // (eğer kol gövdesi sonlandırmıyorsa)
        // Match ifadesi statement olarak kullanılıyorsa sonlandırma kontrolü yapılır.
        // Match bir ifadeyse, atlama öncesi sonucu match ifadesinin sonuç kaydına kopyala.
          currentIrBlock.instructions ~= { opCode: IrOpCode.Copy, dest: matchResultReg, src1: armResultReg }; // Varsayım: Sonuç kaydı kopyalama
         armBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: endOfMatchBlockId };
         writeln("Kontrol Akışı: Match kolu sonrası match sonuna atlama eklendi.");
    }


    // 6. Match yapısından sonraki kod için blok (Match sonu)
    IrBlock* endBlock = &irProgram.blocks[endOfMatchBlockId];
    endBlock.id = endOfMatchBlockId; // ID'yi set et
    writeln("Match sonu bloğu oluşturuldu. ID: ", endOfMatchBlockId);

    // Kod üretim fonksiyonu, genellikle yeni akışın devam ettiği bloğu döndürür.
    return endBlock;
}

// generateExpressionCode fonksiyonu expression_codegen.d'den gelir.
// irProgram.nextRegister() sanal kayıt ID'leri atamak için bir yardımcı fonksiyon olabilir.
// generateCodeForMatchArmBody fonksiyonu match kolunun gövdesini (statement veya ifade) üretecek.