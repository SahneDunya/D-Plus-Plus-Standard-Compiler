module lifetime_analyzer;

import std.stdio;
import std.array;
import std.string;
import std.typecons; // Tuple için
import ast;           // AST düğüm yapıları
import syntax_kind;   // SyntaxKind enum'ı
import dpp_types;     // D++ tür temsilcileri
import symbol_table;  // Symbol yapısı
import scope_manager; // Kapsam bilgisi için (yaşam süreleri kapsamlarla ilişkili olabilir)
import error_reporting; // Hata raporlama için
import control_flow_analysis; // Kontrol akışı grafiği için (gereklidir!)

// Basit bir "Ödünç Alma" (Borrow) bilgisini temsil eden yapı
// Gerçekte bu yapı, ödünç almanın türünü (& veya &mut), yaşam süresi bilgisini
// ve ödünç almanın yapıldığı kod konumunu içermelidir.
struct BorrowInfo {
    bool isMutable; // Mutable ödünç alma mı?
     string lifetime; // Ödünç almanın geçerli olduğu yaşam süresi (basitlik için scope ID'si olabilir)
    int borrowScopeId; // Ödünç almanın yapıldığı kapsamın ID'si (ScopeManager'dan)
     ASTNode* borrowExpressionNode; // Ödünç almanın yapıldığı AST düğümü
}

// Analiz sırasında değişkenlerin veya bellek konumlarının ödünç alma durumlarını takip eden harita
// Çok basitleştirilmiş: Sadece değişken ismine göre takip ediyoruz. Gerçekte bellek konumlarına göre olmalı.
// Bu, her değişken için aktif ödünç alma listesini tutar.
private BorrowInfo[][] activeBorrows; // ScopeStack'e paralel olarak her kapsam için aktif ödünç alma listesi

// Lifetime analizini başlatan ana fonksiyon
// Genellikle semantik analizden SONRA çalıştırılır.
int analyzeLifetimes(ASTNode* syntaxTree, ScopeManager* scopeManager) {
    writeln("Yaşam süresi ve ödünç alma analizi başlatılıyor...");
    if (!syntaxTree) {
        stderr.writeln("Hata (LifetimeAnalyzer): Analiz için AST mevcut değil.");
        return 1;
    }

    int errorsFound = 0;
    // activeBorrows dizisini scopeStack boyutuna göre başlat (global kapsam için ilk eleman)
    activeBorrows = new BorrowInfo[scopeManager.scopeStack.length][];


    // AST üzerinde dolaşarak ödünç alma kurallarını kontrol et
    // Bu da genellikle bir Visitor veya recursive fonksiyonlarla yapılır.
    // Özellikle referans oluşturulan veya kullanılan yerler, fonksiyon çağrıları ve atamalar kritiktir.
    errorsFound += traverseASTForLifetimes(syntaxTree, scopeManager);


    // Analiz sonunda aktif ödünç alma kalmamalıdır (kapsam dışına çıkıldığında bırakılmış olmalı)
    // Bu basit örnekte tam kapsam takibi olmadığı için bu kontrol tam doğru olmaz.

    writeln("Yaşam süresi ve ödünç alma analizi tamamlandı. ", errorsFound, " hata bulundu.");
    return errorsFound; // Toplam hata sayısını döndür
}

// AST'yi yaşam süresi analizi için dolaşan recursive fonksiyon (Çok basit bir taslak)
// Bu fonksiyon, ödünç alma ve bırakma olaylarını izlemeye çalışır
// ve ödünç alma kurallarının ihlallerini tespit etmeye çalışır.
int traverseASTForLifetimes(ASTNode* node, ScopeManager* scopeManager) {
    if (!node) return 0;

    int errors = 0;

    // Kapsam giriş/çıkışlarını izle (activeBorrows dizisini senkronize et)
    bool enteredNewScope = false;
    if (node.kind == SyntaxKind.FunctionDeclarationNode || node.kind == SyntaxKind.BlockStatementNode) {
        // Yeni kapsam girildiğinde activeBorrows için yeni bir seviye ekle
         ScopeManager.enterScope() çağrısı scopeStack boyutunu artırmış olmalı.
        if (activeBorrows.length < scopeManager.scopeStack.length) {
             activeBorrows ~= []; // Yeni kapsam için boş ödünç alma listesi ekle
             enteredNewScope = true;
             writeln("Lifetime Analiz: Yeni kapsam girildi. Active Borrows seviyesi: ", activeBorrows.length - 1);
        } else {
             // Beklenmedik durum, scopeStack boyutu activeBorrows boyutunu geçmedi
             stderr.writeln("Hata (LifetimeAnalyzer): ScopeManager ve activeBorrows senkron değil!");
        }
    }


    // Düğüm türüne göre yaşam süresi/ödünç alma kontrollerini yap
    switch (node.kind) {
        // Değişken Bildirimi (Sahip burada yaratılır)
        case SyntaxKind.VariableDeclarationNode:
            auto varNode = cast(VariableDeclarationNode*)node;
            // Yeni bir değişken (sahip) bildirildiğinde, onun için aktif ödünç alma listesi boştur.
            // Bu bilgi Symbol'e eklenebilir veya ayrı bir veri yapısında tutulabilir.
            addVariableToLifetimeTracker(varNode.name.value, currentScope.id);
             writeln("Lifetime Analiz: Değişken bildirimi: ", varNode.name.value);
            break;

        // Tanımlayıcı Kullanımı (Değişken veya Referans olabilir)
        case SyntaxKind.IdentifierExpressionNode:
            auto identNode = cast(IdentifierExpressionNode*)node;
            // Eğer bu tanımlayıcı bir değişkene referans veriyorsa (semantik analizden biliyoruz)
            // ve bir referans oluşturma bağlamındaysa (örneğin &var veya &mut var)
            // burada yeni bir ödünç alma kaydedilmelidir.
            // Eğer bu tanımlayıcı bir ödünç almayı kullanıyorsa, ödünç almanın hala geçerli olup olmadığı kontrol edilmelidir.

             // Basit Kontrol: Eğer değişken kullanılıyorsa, MUTABLE ödünç alma aktif olmamalı.
             // Bu çok basit bir kontrol, gerçek kurallar çok daha karmaşık.
            Symbol* symbol = scopeManager.resolveSymbol(identNode.value); // Sembolü çöz
            if (symbol && symbol.kind == SyntaxKind.IdentifierToken) { // Eğer değişken ise
                 // Bu değişken için mevcut kapsamlardaki aktif ödünç almaları kontrol et
                 for (int i = activeBorrows.length - 1; i >= 0; --i) {
                     foreach (borrow; activeBorrows[i]) {
                         // Bu ödünç alma bu değişkenle ilgili mi? (Şimdilik isme göre varsayalım)
                         // Gerçekte, ödünç alma bilgisi hangi bellek konumuna ait olduğunu bilmelidir.
                          if (borrow.variableName == identNode.value) { // Varsayım: BorrowInfo'da değişken adı var
                             if (borrow.isMutable) {
                                 // Hata: Değişken kullanılırken mutable ödünç alma aktif!
                                 stderr.writeln("Hata (LifetimeAnalyzer): Satır ", identNode.identifierToken.lineNumber, ": Değişken '", identNode.value, "' kullanılırken mutable ödünç alma aktif.");
                                  error_reporting.reportError(...);
                                 errors++;
                             }
                          }
                     }
                 }
            }
            break;

        // Referans Oluşturma İfadeleri (Örnek: &expr, &mut expr)
        // Bu yapılar AST'de temsil edilmeli ve burada işlenmelidir.
         case SyntaxKind.ReferenceExpressionNode: // Varsayım: Referans ifadesi düğüm türü
            auto refNode = cast(ReferenceExpressionNode*)node;
        //    // Referans verilen ifadeyi (refNode.expression) analiz et.
            errors += traverseASTForLifetimes(refNode.expression, scopeManager);
        //    // Yeni bir ödünç alma kaydet. Bu ödünç alma refNode.expression'a aittir.
        //    // Ödünç almanın yaşam süresi, referansın kendisinin yaşam süresine bağlıdır.
        //    // Örneğin, referansın geçerli olduğu scope.
            Symbol* targetSymbol = null; // refNode.expression'dan hedeflenen sembolü/bellek konumunu bul
            if (targetSymbol) {
                BorrowInfo newBorrow = {
                    isMutable: refNode.isMutable, // &mut ise true
                    borrowScopeId: scopeManager.getCurrentScope().id // Ödünç almanın yapıldığı kapsamın ID'si
                     borrowExpressionNode: node // İsteğe bağlı referans
                };
                // Bu ödünç almayı ilgili değişkene/bellek konumuna kaydet ve kuralları kontrol et.
                // checkAndAddBorrow(targetSymbol, newBorrow); // Karmaşık kontrol fonksiyonu
                 writeln("Lifetime Analiz: Yeni ödünç alma kaydedildi.");
            }
            break;

        // Atama İfadeleri (=)
        case SyntaxKind.BinaryOperatorExpressionNode: // Atama da ikili operatör olabilir
             auto binOpNode = cast(BinaryOperatorExpressionNode*)node;
             if (binOpNode.operatorToken.kind == SyntaxKind.EqualsToken) { // Atama operatörü
                 // Sol taraf (hedef) değiştirilebilir olmalı (semantik analizde kontrol edilir).
                 // Eğer sol taraf bir değişkense (IdentifierExpressionNode)
                 if (binOpNode.left.kind == SyntaxKind.IdentifierExpressionNode) {
                      auto targetIdentNode = cast(IdentifierExpressionNode*)binOpNode.left;
                     Symbol* targetSymbol = scopeManager.resolveSymbol(targetIdentNode.value);
                     if (targetSymbol && targetSymbol.kind == SyntaxKind.IdentifierToken) {
                         // Atama yapıldığında, hedeflenen değişkenin tüm aktif ödünç almaları İPTAL olur (geçersizleşir).
                         // Rust kuralı: Bir sahip (owner) değiştiğinde, onun tüm ödünç almaları sonlanır.
                         // Bu değişkenle ilgili aktif ödünç almaları temizle.
                          clearBorrowsForVariable(targetSymbol); // Yardımcı fonksiyon
                          writeln("Lifetime Analiz: Atama sonrası ödünç almalar temizlendi: ", targetSymbol.name);
                     }
                 }
             }
             // İşlenenleri dolaşmaya devam et
            errors += traverseASTForLifetimes(binOpNode.left, scopeManager);
            errors += traverseASTForLifetimes(binOpNode.right, scopeManager);
            break;


        // Fonksiyon Çağrıları
        case SyntaxKind.CallExpressionNode:
             auto callNode = cast(CallExpressionNode*)node;
            // Fonksiyon çağrıları, argümanların sahipliğini/ödünç almalarını etkileyebilir.
            // Argümanlar sahipliği transfer edebilir (move), mutable veya immutable ödünç alınabilir.
            // Bu, fonksiyon imzasındaki parametre türlerine (sahip, &T, &mut T) bağlıdır.
            // Örneğin, eğer bir fonksiyon mutable referans alıyorsa (&mut T), çağrı noktasında
            // bu referansın kuralları kontrol edilmeli ve fonksiyon çağrısı süresince ilgili değişken
            // başka bir şekilde ödünç alınamaz veya kullanılamaz olmalıdır.
            // Bu da kontrol akışı ve veri akışı analizi gerektirir (karmaşık!).
             writeln("Lifetime Analiz: Fonksiyon çağrısı (", callNode.function.kind.to!string, ") - Ödünç alma etkileşimi kontrolü gerekli.");
            // Argümanları dolaş
            errors += traverseASTForLifetimes(callNode.function, scopeManager);
            foreach (arg; callNode.arguments) {
                errors += traverseASTForLifetimes(arg, scopeManager);
            }
            break;


        // Kod Blokları ({ ... })
        case SyntaxKind.BlockStatementNode:
            // Blok sonuna gelindiğinde, bu bloğun kapsamındaki ödünç almalar SONLANIR.
            // Bu, exitScope ile senkronize olmalıdır.
            // Ancak bu fonksiyon sadece dolaşıyor, exitScope traverseAST fonksiyonunda yönetiliyor.
            // Ödünç almaları temizleme mantığı exitScope'da veya burada dolaşma sonrası yapılabilir.
             writeln("Lifetime Analiz: Blok sonu - Kapsamdaki ödünç almaların sonlandırılması gerekli.");
            // Blok içindeki statement'ları dolaş
             foreach (stmt; node.children) {
                errors += traverseASTForLifetimes(stmt, scopeManager);
            }
            break;

        // ... Diğer AST düğüm türleri için yaşam süresi/ödünç alma mantığı
        default:
            // Bilinmeyen veya işlenmeyen düğüm türleri için çocukları dolaşmaya devam et
            foreach (child; node.children) {
                 errors += traverseASTForLifetimes(child, scopeManager);
            }
            break;
    }

    // Kapsamdan çıkışta ödünç almaları temizle (eğer bu düğüm yeni bir kapsam başlattıysa)
    if (enteredNewScope) {
        // ScopeManager.exitScope() çağrısı ScopeManager'ın stack'ini zaten azaltmış olmalı.
        // activeBorrows'tan ilgili kapsam seviyesini çıkar veya temizle.
         activeBorrows.popBack(); // Kapsam çıkışına denk gelen seviyeyi çıkar
         writeln("Lifetime Analiz: Kapsamdan çıkıldı - Ödünç almalar temizleniyor.");
         // Bu kapsamdaki tüm ödünç almaları temizle.
          clearBorrowsInScope(scopeManager.scopeStack.length); // Yardımcı fonksiyon
    }


    return errors; // Toplam hata sayısını döndür
}

// Yardımcı Fonksiyonlar (Çok basitleştirilmiş konseptler)

// Belirli bir değişkenle ilgili tüm aktif ödünç almaları temizler
void clearBorrowsForVariable(Symbol* variableSymbol) {
    if (!variableSymbol) return;
    // Gerçekte, bu, bellek konumuna göre ödünç almaları bulup kaldırmalıdır.
    // Basitlik için, activeBorrows dizisinde değişken adına göre arayalım (DOĞRU DEĞİL!)
     for (int i = 0; i < activeBorrows.length; ++i) {
         activeBorrows[i] = activeBorrows[i].filter!(borrow => borrow.variableName != variableSymbol.name true).array; // Variable name varsayımı
     }
}

// Belirli bir kapsam seviyesindeki tüm aktif ödünç almaları temizler
void clearBorrowsInScope(int scopeLevel) {
    if (scopeLevel >= 0 && scopeLevel < activeBorrows.length) {
        activeBorrows[scopeLevel] = []; // İlgili kapsam seviyesindeki listeyi boşalt
    }
}

// Yeni bir ödünç alma ekler ve ödünç alma kurallarını kontrol eder.
// Symbol* targetSymbol: Ödünç alınan değişken/bellek konumu
// BorrowInfo newBorrow: Yeni ödünç alma bilgisi
 int checkAndAddBorrow(Symbol* targetSymbol, BorrowInfo newBorrow) {
//     // Bu değişken için mevcut tüm aktif ödünç almaları al.
//     // Eğer yeni ödünç alma mutable (&mut) ise:
//     // - Başka hiçbir aktif ödünç alma olmamalı (& veya &mut).
//     // Eğer yeni ödünç alma immutable (&) ise:
//     // - Başka hiçbir mutable (&mut) aktif ödünç alma olmamalı.
//
//     // Eğer kural ihlali varsa hata raporla.
//     // Eğer ihlal yoksa, yeni ödünç almayı ilgili değişkenin aktif ödünç alma listesine ekle.
//     // Bu liste de yaşam süresi ve kapsam bilgisi içermelidir.

      stderr.writeln("Uyarı: checkAndAddBorrow fonksiyonu implemente edilmedi (çok karmaşık!).");
      return 0; // Placeholder
 }