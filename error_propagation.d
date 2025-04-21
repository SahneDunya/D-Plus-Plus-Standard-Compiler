module error_propagation;

import std.stdio;
import ast;           // AST düğüm yapıları (CallExpressionNode, PostfixExpressionNode gibi)
import syntax_kind;   // SyntaxKind enum'ı
import dpp_types;     // D++ tür temsilcileri (Result, Option, FunctionType gibi)
import symbol_table;  // Sembol bilgisi için (current function'ın dönüş tipi)
import scope_manager; // Kapsam bilgisi için
import type_system;   // Tür sistemi kuralları (tip kontrolü, uyumluluk)
import ir;            // Ara Temsil yapıları (IrOpCode, IrInstruction, IrBlock)
import expression_codegen; // İşlenen ifade için kod üretimi


// Bir ifade üzerindeki '?' operatörünü anlamsal olarak analiz eden ve IR kodu üreten fonksiyon
// Genellikle semantic_analyzer'daki AST traversi sırasında PostfixExpressionNode (expr?) ile karşılaşınca çağrılır.
// İfadenin (expr?) belirlenen tipini döndürür ve hata varsa raporlar.
// node: İşlenecek PostfixExpressionNode (expr düğümünü ve '?' tokenını içerir)
// currentIrBlock: Şu anda IR komutlarının ekleneceği IR bloğu
// irProgram: Genel IR program yapısı (yeni bloklar eklemek için)
// scopeManager: Kapsam bilgisi için (mevcut fonksiyonun dönüş tipini bulmak için)
// Bu fonksiyonun, '?' operatörünü temsil eden PostfixExpressionNode'u işlediği varsayılır.
DppType* analyzeAndGenerateQuestionMark(PostfixExpressionNode* node, IrBlock* currentIrBlock, IrProgram* irProgram, ScopeManager* scopeManager) {
    if (!node || !node.operand || node.operatorToken.kind != SyntaxKind.QuestionMarkToken || !currentIrBlock || !irProgram || !scopeManager) {
        // Hata: Geçersiz düğüm veya context
        stderr.writeln("Hata (ErrorPropagation): Geçersiz '?' operatörü kod üretimi düğümü veya context.");
         error_reporting.reportError(...);
        return DppType.Unknown;
    }

    writeln("'?' operatörü analizi ve kodu üretiliyor...");

    int errors = 0;

    // 1. İşlenen ifade için kod üret ve tipini belirle
    // İşlenenin tipi bir Result<T, E> veya Option<T> olmalıdır.
    Register operandReg = generateExpressionCode(node.operand, currentIrBlock, irProgram, scopeManager); // İşlenenin değerini hesapla
    DppType* operandType = checkType(node.operand, scopeManager.getCurrentScope()); // İşlenenin tipini belirle


    // İşlenenin tipinin Result veya Option olup olmadığını kontrol et
    if (operandType.kind != DppTypeKind.Result && operandType.kind != DppTypeKind.Option) {
        stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": '?' operatörü sadece Result veya Option tipleri üzerinde kullanılabilir, ancak tipi '", operandType.name, "'.");
         error_reporting.reportError(...);
        errors++;
        return DppType.Unknown;
    }

    // 2. '?' operatörünün kullanıldığı mevcut fonksiyonun dönüş tipini belirle
    // '?' operatörü, hata/None değerini mevcut fonksiyondan döndürmek için kullanılır.
    // Bu yüzden mevcut fonksiyonun dönüş tipini bilmek önemlidir.
    // ScopeManager'dan mevcut fonksiyon sembolünü bulabiliriz.
    Symbol* currentFunctionSymbol = null;
    // ScopeManager'da "current function" bilgisini tutmak veya AST üzerinde yukarı çıkarak bulmak gerekebilir.
    // Basitlik için, mevcut kapsamın bir fonksiyon kapsamı olduğunu varsayalım ve sembolünü bulalım.
    // Gerçek implementasyonda, AST traversi sırasında fonksiyon düğümüne girildiğinde bu bilgi kaydedilmelidir.
     SymbolTable* currentScope = scopeManager.getCurrentScope();
     currentFunctionSymbol = findEnclosingFunctionSymbol(scopeManager); // Yardımcı fonksiyon gerekli

     if (!currentFunctionSymbol || currentFunctionSymbol.type.kind != DppTypeKind.Function) {
         stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": '?' operatörü sadece fonksiyonların içinde kullanılabilir.");
          error_reporting.reportError(...);
         errors++;
         return DppType.Unknown;
     }
    auto currentFunctionReturnType = cast(FunctionType*)currentFunctionSymbol.type.returnType; // Mevcut fonksiyonun dönüş tipi


    // 3. İşlenenin tipine göre dallanma ve kod üretimi
    Register resultReg = irProgram.nextRegister(); // Başarılı durumda çıkarılan değer için kayıt

    if (operandType.kind == DppTypeKind.Result) {
        auto resultType = cast(ResultType*)operandType;
        // Result<T, E> durumunda:
        // - Eğer Ok(value) ise, value'yu çıkar ve resultReg'e koy.
        // - Eğer Err(error) ise, error'u çıkar ve mevcut fonksiyondan error ile dön.

        // Ok durumunda atlanacak blok
        int okBlockId = irProgram.nextBlockId();
        // Err durumunda gidilecek blok (burada erken dönüş kodu üretilecek)
        int errBlockId = irProgram.nextBlockId();
        // '?' ifadesinden sonraki kod için blok
        int afterQuestionMarkBlockId = irProgram.nextBlockId();

        // Gerekli bloklar için yer ayır
        irProgram.blocks.length = afterQuestionMarkBlockId + 1;

        // Result değerinin Err varyantı olup olmadığını kontrol et (discriminator'a bak)
        currentIrBlock.instructions ~= {
            opCode: IrOpCode.CheckResultIsErr, // Varsayım: Result'ın Err olup olmadığını kontrol eden IR komutu
            src1: operandReg,                // Kontrol edilecek Result değeri
            labelId: errBlockId              // Eğer Err ise gidilecek etiket
        };
        // Eğer Err değilse (yani Ok ise), sonraki komut çalışır ve Ok bloğuna atlanır.
         currentIrBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: okBlockId };


        // Ok bloğu: Başarılı değeri çıkar
        IrBlock* okBlock = &irProgram.blocks[okBlockId];
        okBlock.id = okBlockId;
        // Ok değerini Result'tan çıkar ve resultReg'e koy
        okBlock.instructions ~= {
            opCode: IrOpCode.ExtractOkValue, // Varsayım: Result'tan Ok değerini çıkaran IR komutu
            dest: resultReg,
            src1: operandReg
        };
        // Ok bloğunun sonundan '?' sonrası bloğuna atla
        okBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: afterQuestionMarkBlockId };


        // Err bloğu: Hata değerini çıkar ve erken dön
        IrBlock* errBlock = &irProgram.blocks[errBlockId];
        errBlock.id = errBlockId;
        // Hata değerini Result'tan çıkar
        Register errorValueReg = irProgram.nextRegister(); // Hata değeri için kayıt
        errBlock.instructions ~= {
            opCode: IrOpCode.ExtractErrValue, // Varsayım: Result'tan Err değerini çıkaran IR komutu
            dest: errorValueReg,
            src1: operandReg
        };

        // Mevcut fonksiyonun dönüş tipinin, yayılacak hata tipiyle uyumlu olup olmadığını kontrol et
        // Current Function Return Type: Result<T_fn, E_fn> olmalı
        // Propagated Error Type: E olmalı (Result<T, E>'den gelen)
        // E, E_fn'e atanabilir olmalı (From<E> for E_fn implementasyonu gibi)
         if (currentFunctionReturnType.kind != DppTypeKind.Result) {
              stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": '?' operatörü Result üzerinde kullanıldı, ancak kapsayan fonksiyon Result döndürmüyor.");
               error_reporting.reportError(...);
              errors++;
              return DppType.Unknown;
         }
         auto currentFuncResultType = cast(ResultType*)currentFunctionReturnType;
         auto propagatedErrorType = resultType.errType; // Propagated error type E
         auto expectedErrorType = currentFuncResultType.errType; // Expected error type E_fn

         if (!propagatedErrorType.isAssignableTo(expectedErrorType)) { // Tür sistemi kullanarak kontrol et
              stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": Result hatası tipi ('", propagatedErrorType.name, "') kapsayan fonksiyonun hata tipiyle ('", expectedErrorType.name, "') uyumsuz.");
              error_reporting.reportError(...);
              errors++;
              return DppType.Unknown;
         }

        // Hata değeri ile fonksiyondan dön
        errBlock.instructions ~= {
            opCode: IrOpCode.ReturnWithError, // Varsayım: Hata değeri ile dönen IR komutu
            src1: errorValueReg              // Döndürülecek hata değeri
            // Veya: Önce hata değerini kapsayan fonksiyonun dönüş tipine (Result) paketle ve sonra dön.
             Register packagedErrorReg = irProgram.nextRegister();
             errBlock.instructions ~= { opCode: IrOpCode.PackageError, dest: packagedErrorReg, src1: errorValueReg }; // Varsayım
             errBlock.instructions ~= { opCode: IrOpCode.Return, src1: packagedErrorReg };
        };


        // '?' ifadesinin tipi, Result'ın Ok değerinin tipidir (T).
          node.resolvedType = resultType.okType; // Gelişmiş AST
         return resultType.okType;

    } else if (operandType.kind == DppTypeKind.Option) {
         auto optionType = cast(OptionType*)operandType;
        // Option<T> durumunda:
        // - Eğer Some(value) ise, value'yu çıkar ve resultReg'e koy.
        // - Eğer None ise, mevcut fonksiyondan None ile dön.

        // Some durumunda atlanacak blok
        int someBlockId = irProgram.nextBlockId();
        // None durumunda gidilecek blok (burada erken dönüş kodu üretilecek)
        int noneBlockId = irProgram.nextBlockId();
        // '?' ifadesinden sonraki kod için blok (Result ile aynı)
        int afterQuestionMarkBlockId = irProgram.nextBlockId();

        // Gerekli bloklar için yer ayır
        irProgram.blocks.length = afterQuestionMarkBlockId + 1;


        // Option değerinin None varyantı olup olmadığını kontrol et
        currentIrBlock.instructions ~= {
            opCode: IrOpCode.CheckOptionIsNone, // Varsayım: Option'ın None olup olmadığını kontrol eden IR komutu
            src1: operandReg,                 // Kontrol edilecek Option değeri
            labelId: noneBlockId              // Eğer None ise gidilecek etiket
        };
        // Eğer None değilse (yani Some ise), sonraki komut çalışır ve Some bloğuna atlanır.
         currentIrBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: someBlockId };


        // Some bloğu: Başarılı değeri çıkar
        IrBlock* someBlock = &irProgram.blocks[someBlockId];
        someBlock.id = someBlockId;
        // Some değerini Option'dan çıkar ve resultReg'e koy
        someBlock.instructions ~= {
            opCode: IrOpCode.ExtractSomeValue, // Varsayım: Option'dan Some değerini çıkaran IR komutu
            dest: resultReg,
            src1: operandReg
        };
        // Some bloğunun sonundan '?' sonrası bloğuna atla
        someBlock.instructions ~= { opCode: IrOpCode.Jump, labelId: afterQuestionMarkBlockId };


        // None bloğu: None ile erken dön
        IrBlock* noneBlock = &irProgram.blocks[noneBlockId];
        noneBlock.id = noneBlockId;

        // Mevcut fonksiyonun dönüş tipinin Option<T_fn> olup olmadığını kontrol et
        // Propagated value: None
        // Current Function Return Type: Option<T_fn> olmalı
        // T_fn'in T'ye uyumluluğu da kontrol edilebilir (gerekiyorsa)
         if (currentFunctionReturnType.kind != DppTypeKind.Option) {
              stderr.writeln("Hata: Satır ", node.operatorToken.lineNumber, ": '?' operatörü Option üzerinde kullanıldı, ancak kapsayan fonksiyon Option döndürmüyor.");
               error_reporting.reportError(...);
              errors++;
              return DppType.Unknown;
         }
        // None değeri ile fonksiyondan dön
        noneBlock.instructions ~= {
            opCode: IrOpCode.ReturnWithNone, // Varsayım: None değeri ile dönen IR komutu
            // Veya: Önce None değerini kapsayan fonksiyonun dönüş tipine (Option) paketle ve sonra dön.
             Register packagedNoneReg = irProgram.nextRegister();
             noneBlock.instructions ~= { opCode: IrOpCode.PackageNone, dest: packagedNoneReg }; // Varsayım
             noneBlock.instructions ~= { opCode: IrOpCode.Return, src1: packagedNoneReg };
        };


        // '?' ifadesinin tipi, Option'ın Some değerinin tipidir (T).
          node.resolvedType = optionType.someType; // Gelişmiş AST
         return optionType.someType;

    } else {
        // Bu duruma üstteki kontrolde hata verildi.
        return DppType.Unknown;
    }

    // '?' ifadesinden sonraki kodun başlangıcı
    IrBlock* afterQuestionMarkBlock = &irProgram.blocks[afterQuestionMarkBlockId];
    afterQuestionMarkBlock.id = afterQuestionMarkBlockId;

    // Kod üretim fonksiyonu, genellikle yeni akışın devam ettiği bloğu döndürür.
    // '?' ifadesi bir değer ürettiği için, bu değer resultReg'e konulmuştur.
    // Kod akışı afterQuestionMarkBlock'tan devam edecektir.
     return afterQuestionMarkBlock.id; // Bu blok ID'sini döndürelim (IR.d'deki gibi blok döndürmek daha iyi)
      return afterQuestionMarkBlock; // IR.d'deki gibi IrBlock* döndürseydik
}


// checkType fonksiyonu type_checker.d'den gelir.
// generateExpressionCode fonksiyonu expression_codegen.d'den gelir.
// irProgram.nextRegister() ve irProgram.nextBlockId() IR.d'deki helper'lardır.
// findEnclosingFunctionSymbol fonksiyonu scope_manager.d'de veya semantic_analyzer.d'de bulunabilir.

// '?' operatörünün AST'de nasıl temsil edildiği önemlidir.
// Genellikle bir PostfixExpressionNode (operand, operatorToken) olarak ele alınır.

class PostfixExpressionNode : ExpressionNode {
    ASTNode* operand; // Üzerinde operatör kullanılan ifade
    const(Token) operatorToken; // Operatör tokenı ('?')

    this(ASTNode* operand, const(Token) operatorToken) {
        super(SyntaxKind.PostfixExpressionNode);
        this.operand = operand;
        this.operatorToken = operatorToken;
        addChild(operand);
    }
}
