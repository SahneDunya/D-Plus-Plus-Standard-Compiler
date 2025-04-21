module semantic_analyzer;

import std.stdio;
import std.array;
import std.string;
import ast; // AST düğüm yapıları
import syntax_kind; // SyntaxKind enum'ı
import dpp_types; // D++ tür temsilcileri
import type_checker; // type_checker.d modülü
import error_reporting; // Hata raporlama için (ileride eklenecek)

// Sembol tablosu girişi için temel yapı
struct Symbol {
    string name; // Sembolün adı
    DppType type; // Sembolün türü (değişken tipi, fonksiyon dönüş tipi vb.)
    SyntaxKind kind; // Sembolün türü (Variable, Function, Type vb.)
    // Sembolün bildirildiği AST düğümüne referans
     ASTNode* declarationNode;
    // Değişkenler için offset bilgisi (backend için)
     int offset;
    bool isMutable; // Değişkenler için değiştirilebilir mi?
}

// Kapsamları (scopes) ve sembolleri yöneten sembol tablosu
class SymbolTable {
    Symbol[] symbols; // Bu kapsamdaki semboller
    SymbolTable* parent; // Üst kapsam (nested scopes için)

    this(SymbolTable* parent = null) {
        this.parent = parent;
    }

    // Sembol tablosuna yeni bir sembol ekler
    void addSymbol(Symbol symbol) {
        // Aynı kapsamda aynı isimde sembol var mı kontrolü yapılmalıdır (hata durumu)
        if (resolve(symbol.name, false)) { // Sadece mevcut kapsamda ara
            stderr.writeln("Hata: Sembol '", symbol.name, "' zaten mevcut.");
             error_reporting.reportError(...);
            return;
        }
        symbols ~= symbol;
        writeln("Sembol eklendi: ", symbol.name, " (", symbol.type.name, ")");
    }

    // Bir sembolü mevcut kapsamda veya üst kapsamlarda arar
    // lookInCurrentScopeOnly: Sadece mevcut kapsamda aramak için true
    Symbol* resolve(string name, bool lookInCurrentScopeOnly = true) {
        // Mevcut kapsamda ara
        foreach (ref symbol; symbols) {
            if (symbol.name == name) {
                return &symbol;
            }
        }

        // Eğer sadece mevcut kapsamda aramıyorsak ve üst kapsam varsa, üst kapsamda ara
        if (!lookInCurrentScopeOnly && parent) {
            return parent.resolve(name, false); // Üst kapsamda aramaya devam et
        }

        return null; // Sembol bulunamadı
    }
}

// Semantik analizi yürüten ana fonksiyon
int analyzeSemantics(ASTNode* syntaxTree) {
    writeln("Semantik analiz aşaması başlatılıyor...");
    if (!syntaxTree) {
        stderr.writeln("Hata: Semantik analiz için AST mevcut değil.");
        return 1;
    }

    // Global kapsamı oluştur
    SymbolTable* globalScope = new SymbolTable();
    // Standart kütüphane fonksiyonları ve türleri global kapsamda önceden tanımlanabilir.
     globalScope.addSymbol({name: "println", type: VoidType, kind: SyntaxKind.FnKeyword}); // Örnek: println fonksiyonu

    // AST üzerinde dolaşarak semantik kontrolleri yap
    // Bu genellikle bir "Visitor" deseni veya recursive fonksiyonlarla yapılır.
    // Her düğüm tipi için ayrı işleme mantığı olacaktır.
    int errorsFound = traverseAST(syntaxTree, globalScope); // AST'yi dolaşmaya başla

    // Global kapsamı temizle (eğer D'nin GC'si kullanılmıyorsa manuel)
     destroySymbolTable(globalScope); // İleride yazılacak fonksiyon

    if (errorsFound > 0) {
        stderr.writeln("Semantik analiz tamamlandı. ", errorsFound, " hata bulundu.");
        return 1; // Hata kodu
    }

    writeln("Semantik analiz başarıyla tamamlandı. Hata bulunamadı.");
    return 0; // Başarılı
}

// AST'yi dolaşan recursive fonksiyon (Çok basit bir taslak)
// Gerçekte, her düğüm tipi için ayrı logic içeren bir switch/case yapısı veya Visitor kullanılır.
int traverseAST(ASTNode* node, SymbolTable* currentScope) {
    if (!node) return 0;

    int errors = 0;

    // Kapsam giriş/çıkışlarını yönet
    SymbolTable* nextScope = currentScope;
    bool enteredNewScope = false;

    // Belirli düğüm türleri yeni kapsam başlatır (örn: fonksiyon gövdesi, blok statement)
    if (node.kind == SyntaxKind.FunctionDeclarationNode || node.kind == SyntaxKind.BlockStatementNode) {
        nextScope = new SymbolTable(currentScope); // Yeni alt kapsam oluştur
        enteredNewScope = true;
        writeln("Yeni kapsam girildi (", node.kind.to!string, ")");
    }


    // Düğüm türüne göre anlamsal işlemleri yap
    switch (node.kind) {
        case SyntaxKind.ProgramNode:
            // Program düğümü için özel bir işlem olmayabilir, sadece çocuklarını dolaş.
            break;

        case SyntaxKind.FunctionDeclarationNode:
            auto funcNode = cast(FunctionDeclarationNode*)node;
            // Fonksiyon adını sembol tablosuna ekle (mevcut kapsama)
            // Dönüş tipi ve parametre tipleri de Sembol bilgisinde tutulabilir.
            if (funcNode.name) {
                // Fonksiyon tipi oluşturulabilir (parametre tiplerini ve dönüş tipini içeren)
                DppType functionType = {kind: TypeKind.Unknown, name: funcNode.name.value}; // Placeholder tip
                if (funcNode.returnType) {
                    functionType.name = funcNode.name.value ~ " -> " ~ funcNode.returnType.type.name; // Basit temsil
                }

                 currentScope.addSymbol({
                    name: funcNode.name.value,
                    type: functionType, // Gerçek fonksiyon tipi
                    kind: SyntaxKind.FnKeyword, // Sembol türü: Fonksiyon
                    isMutable: false // Fonksiyonlar genellikle mutable değildir
                 });
            }

            // Fonksiyonun parametrelerini yeni kapsamda sembol tablosuna ekle
            if (enteredNewScope) { // Eğer yeni kapsam oluşturulduysa (funcNode.body yeni kapsamda olur)
                 foreach (param; funcNode.parameters) {
                     if (param.name) {
                        // Parametre tipini belirle (param.type düğümünden)
                        DppType paramType = param.type ? param.type.type : DppType.Unknown; // Varsayım: TypeNode.type mevcut
                         nextScope.addSymbol({
                            name: param.name.value,
                            type: paramType,
                            kind: SyntaxKind.IdentifierToken, // Sembol türü: Değişken/Parametre
                            isMutable: param.isMutable // Parametre mutable olabilir mi?
                         });
                     }
                 }
            }


            // Fonksiyon gövdesini dolaş (bu, yeni kapsamı kullanacak)
            if (funcNode.body) {
                 errors += traverseAST(funcNode.body, nextScope); // Önce gövdeyi işle
            }

            // Parametreleri ve dönüş tipini de dolaş (isteğe bağlı, TypeChecker'da işlenebilir)
             foreach (param; funcNode.parameters) errors += traverseAST(param, nextScope);
             if (funcNode.returnType) errors += traverseAST(funcNode.returnType, nextScope);

            break;

        case SyntaxKind.VariableDeclarationNode:
            auto varNode = cast(VariableDeclarationNode*)node;
            // Değişken adını sembol tablosuna ekle (mevcut veya yeni kapsama)
            if (varNode.name) {
                // Değişkenin tipini belirle (varNode.type düğümünden veya initializer'dan)
                DppType variableType = DppType.Unknown;
                if (varNode.type) {
                    variableType = varNode.type.type; // Belirtilen tip
                } else if (varNode.initializer) {
                    // Tip belirtilmemişse, başlangıç değerinden tipi çıkar (Type Inference)
                    // Bu, TypeChecker'ın bir görevi olabilir.
                    variableType = checkType(varNode.initializer, currentScope); // İleride TypeChecker kullanılacak
                }

                 currentScope.addSymbol({
                    name: varNode.name.value,
                    type: variableType,
                    kind: SyntaxKind.IdentifierToken, // Sembol türü: Değişken
                    isMutable: varNode.isMutable // 'mut' anahtar kelimesine göre belirle
                 });
            }

            // Başlangıç değeri ifadesini dolaş ve tipini kontrol et
            if (varNode.initializer) {
                errors += traverseAST(varNode.initializer, currentScope); // İfadeyi aynı kapsamda işle
                // Başlangıç değerinin tipi, değişkenin tipiyle uyumlu mu kontrol et (TypeChecker)
                 DppType initializerType = checkType(varNode.initializer, currentScope); // İleride kullanılacak
                 if (variableType.kind != TypeKind.Unknown && !variableType.isEquivalent(initializerType)) {
                     stderr.writeln("Hata: Satır ", varNode.name.identifierToken.lineNumber, ": Değişken tipi ('", variableType.name, "') başlangıç değeri tipiyle ('", initializerType.name, "') uyumsuz.");
                      error_reporting.reportError(...);
                     errors++;
                 }
            }
            break;

        case SyntaxKind.IdentifierExpressionNode:
            auto identNode = cast(IdentifierExpressionNode*)node;
            // Tanımlayıcının sembol tablosunda olup olmadığını kontrol et (İsim Çözümlemesi)
            Symbol* symbol = currentScope.resolve(identNode.value, false); // Üst kapsamlarda da ara
            if (!symbol) {
                stderr.writeln("Hata: Satır ", identNode.identifierToken.lineNumber, ", Sütun ", identNode.identifierToken.columnNumber, ": Tanımlayıcı '", identNode.value, "' bulunamadı.");
                // error_reporting.reportError(...);
                errors++;
                // Tanımlayıcının tipini UnknownType olarak işaretle
                 identNode.resolvedType = DppType.Unknown; // AST düğümüne çözümlenmiş tipi ekle (gelişmiş AST)
            } else {
                // Tanımlayıcının çözümlenmiş sembolüne referansı AST düğümüne ekle
                 identNode.resolvedSymbol = symbol; // Gelişmiş AST
                // Tanımlayıcının tipini AST düğümüne ekle
                 identNode.resolvedType = symbol.type; // Gelişmiş AST
                writeln("Tanımlayıcı çözüldü: ", identNode.value, " (Tip: ", symbol.type.name, ")");
            }
            break;

        case SyntaxKind.BinaryOperatorExpressionNode:
            auto binOpNode = cast(BinaryOperatorExpressionNode*)node;
            // Sol ve sağ işlenenleri dolaş
            errors += traverseAST(binOpNode.left, currentScope);
            errors += traverseAST(binOpNode.right, currentScope);
            // İkili operatör ifadesinin tipini kontrol et ve belirle (TypeChecker)
             DppType resultType = checkBinaryOperatorType(binOpNode, currentScope); // İleride kullanılacak
             if (resultType.kind == TypeKind.Unknown) {
                  stderr.writeln("Hata: Satır ", binOpNode.operatorToken.lineNumber, ": İkili operatör ('", binOpNode.operatorToken.value, "') için tip hatası.");
                   error_reporting.reportError(...);
                  errors++;
             }
            // İfadenin çözümlenmiş tipini AST düğümüne ekle
             binOpNode.resolvedType = resultType; // Gelişmiş AST

            break;

        case SyntaxKind.LiteralExpressionNode:
             auto literalNode = cast(LiteralExpressionNode*)node;
            // Literal düğümlerin tipi token türünden belirlenir (TypeChecker'da işlenebilir)
             DppType literalType = checkType(literalNode, currentScope); // İleride kullanılacak
             literalNode.resolvedType = literalType; // Gelişmiş AST
            break;

        case SyntaxKind.IfStatementNode:
            auto ifNode = cast(IfStatementNode*)node;
            // Koşul ifadesini dolaş ve tipini kontrol et (Boolean olmalı)
            errors += traverseAST(ifNode.condition, currentScope);
            DppType conditionType = checkType(ifNode.condition, currentScope); // İleride kullanılacak
            if (conditionType.kind != TypeKind.Bool) {
                 stderr.writeln("Hata: Satır ", ifNode.condition.kind.to!string, ": If koşulu boolean olmalı, ancak tipi '", conditionType.name, "'.");
                  error_reporting.reportError(...);
                 errors++;
            }

            // Then bloğunu dolaş (yeni kapsamı kullanacak)
             errors += traverseAST(ifNode.thenBlock, nextScope);

            // Else dalını dolaş (eğer varsa ve yeni kapsamı kullanacak)
             if (ifNode.elseBranch) {
                 errors += traverseAST(ifNode.elseBranch, nextScope);
             }
            break;

        case SyntaxKind.BlockStatementNode:
            // Blok içindeki statement'ları dolaş (yeni kapsamı kullanacak)
             foreach (stmt; node.children) { // Basit AST'de çocukları statements olarak varsayalım
                errors += traverseAST(stmt, nextScope);
            }
            break;

        // ... Diğer AST düğüm türleri için anlamsal kontrol ve dolaşma mantığı
        default:
            // Bilinmeyen veya işlenmeyen düğüm türleri için çocukları dolaşmaya devam et
            // Gelişmiş AST'de: foreach (child; node.getChildren()) errors += traverseAST(child, nextScope);
            // Basit AST'de:
            foreach (child; node.children) {
                 errors += traverseAST(child, nextScope);
            }
            break;
    }

    // Kapsamdan çık (eğer bu düğüm yeni bir kapsam başlattıysa)
    if (enteredNewScope) {
        writeln("Kapsamdan çıkıldı (", node.kind.to!string, ")");
        // Alt kapsamın sembol tablosunu temizle (eğer GC yoksa)
         destroySymbolTable(nextScope);
    }


    return errors; // Toplam hata sayısını döndür
}

// Sembol tablosunu temizleyen fonksiyon (Manuel bellek yönetimi için)
 void destroySymbolTable(SymbolTable* table) {
     if (!table) return;
     // Alt sembol tablolarını temizle (eğer SymbolTable children tutuyorsa)
     // Sembollerin kendilerini temizle (eğer pointer içeriyorlarsa)
     destroy(table);
 }