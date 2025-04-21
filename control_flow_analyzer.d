module control_flow_analyzer;

import std.stdio;
import std.array;
import ast;           // AST düğüm yapıları
import syntax_kind;   // SyntaxKind enum'ı
import error_reporting; // Hata raporlama için

// Kodun belirli bir noktasının erişilebilir olup olmadığını takip etmek için durum enum'ı
enum ReachabilityStatus {
    Reachable,       // Bu noktaya ulaşılabilir
    Unreachable,     // Bu noktaya ulaşılamaz (dead code)
    MightBeReachable // Koşullu dallanma nedeniyle ulaşılabilir veya ulaşılamaz olabilir
}

// Bir kod bloğunun veya fonksiyon gövdesinin kontrol akışını analiz eden ana fonksiyon
// Genellikle semantic_analyzer tarafından fonksiyon veya blok düğümleri için çağrılır.
// Hata veya uyarı bulunursa sayısını döndürür.
int analyzeControlFlow(ASTNode* blockNode) {
    if (!blockNode || (blockNode.kind != SyntaxKind.BlockStatementNode && blockNode.kind != SyntaxKind.ProgramNode)) {
        // Hata: Analiz için geçerli bir blok veya program düğümü değil
        stderr.writeln("Hata (ControlFlowAnalyzer): Geçersiz kontrol akışı analiz düğümü.");
         error_reporting.reportError(...);
        return 1;
    }

    writeln("Kontrol akışı analizi başlatılıyor...");
    int errorsFound = 0;

    // Blok içindeki statement'ları dolaşarak erişilebilirliği takip et
    ReachabilityStatus currentStatus = ReachabilityStatus.Reachable;

    // Basit bir erişilebilirlik analizi: Statement'ları sırayla işle
    foreach (stmt; blockNode.children) { // BlockStatementNode'un çocukları statement'lardır
        // Eğer mevcut durum Unreachable ise, bu statement'a ulaşılamaz
        if (currentStatus == ReachabilityStatus.Unreachable) {
             stderr.writeln("Uyarı: Satır ...: Erişilemeyen kod bulundu."); // Tam konum bilgisi için AST düğümünü kullan
             error_reporting.reportWarning(...);
            errorsFound++;
        }

        // Statement'ın türüne göre erişilebilirlik durumunu güncelle
        switch (stmt.kind) {
            case SyntaxKind.ReturnStatementNode:
                // Return statement'ından sonraki kodlar erişilemez olur
                currentStatus = ReachabilityStatus.Unreachable;
                break;

            case SyntaxKind.IfStatementNode:
                auto ifNode = cast(IfStatementNode*)stmt;
                // If statement'ı kontrol akışını böler.
                // Then ve else dallarını recursive olarak analiz et.
                // Sonuç, dalların erişilebilirliğine bağlıdır.

                // Basitlik için: Eğer koşul sabit false ise then dalı erişilemez olabilir.
                // Eğer koşul sabit true ise else dalı erişilemez olabilir.
                // Eğer her iki dal da return/break/continue içeriyorsa, if statement'ından sonraki kod erişilemez olabilir.

                // Çok basit bir yaklaşım: Her iki dal da mevcutsa ve ikisi de yürütmeyi sonlandırıyorsa (örn: return içeriyorsa)
                // if statement'ından sonraki kod erişilemez olur.
                 bool thenTerminates = doesStatementTerminateExecution(ifNode.thenBlock); // Yardımcı fonksiyon (aşağıda taslak)
                 bool elseTerminates = ifNode.elseBranch ? doesStatementTerminateExecution(ifNode.elseBranch) : false; // Else dalı varsa kontrol et

                 if (thenTerminates && elseTerminates) {
                     currentStatus = ReachabilityStatus.Unreachable;
                     writeln("Kontrol Akışı: If statement sonrası erişilemez.");
                 } else {
                     // En az bir dal yürütmeyi sonlandırmıyorsa, sonraki kodlara ulaşılabilir.
                     currentStatus = ReachabilityStatus.Reachable; // Durumu sıfırla (basit bir varsayım)
                 }

                // Then ve else dallarını ayrı ayrı analiz et (recursive çağrılar)
                 errorsFound += analyzeControlFlow(ifNode.thenBlock);
                 if (ifNode.elseBranch) {
                     errorsFound += analyzeControlFlow(ifNode.elseBranch);
                 }
                break;

             case SyntaxKind.SwitchStatementNode: // Switch analizi de benzer şekilde dalları kontrol eder
             case SyntaxKind.MatchExpressionNode: // Match analizi de benzer şekilde dalları kontrol eder
             case SyntaxKind.WhileStatementNode: // Döngü analizi daha karmaşıktır
             case SyntaxKind.ForStatementNode:

            default:
                // Çoğu statement (atama, fonksiyon çağrısı gibi) yürütme akışını kesmez.
                // currentStatus aynı kalır.
                // Ancak bu statement'ın çocuk düğümlerini recursive olarak analiz etmeliyiz.
                 foreach (child; stmt.children) {
                     errorsFound += traverseASTForControlFlow(child, currentStatus); // Yardımcı recursive dolaşma fonksiyonu
                 }
                break;
        }
    }


    writeln("Kontrol akışı analizi tamamlandı. ", errorsFound, " uyarı/hata bulundu.");
    return errorsFound;
}

// Bir statement'ın (veya bloğun) yürütmeyi sonlandırıp sonlandırmadığını belirleyen yardımcı fonksiyon
// Basitlik için sadece ReturnStatementNode'u kontrol ediyor.
// Gerçekte BreakStatement, ContinueStatement, ThrowExpression gibi şeyleri de kontrol etmelidir.
bool doesStatementTerminateExecution(ASTNode* node) {
    if (!node) return false;

    // Eğer düğümün kendisi bir sonlandırma statement'ı ise
    if (node.kind == SyntaxKind.ReturnStatementNode) {
        return true;
    }

    // Eğer bir blok ise, içindeki herhangi bir statement sonlandırıyor mu kontrol et
    if (node.kind == SyntaxKind.BlockStatementNode) {
        foreach (stmt; node.children) {
            // Eğer iç içe statement sonlandırıyorsa, blok da sonlandırır
            if (doesStatementTerminateExecution(stmt)) {
                return true;
            }
        }
        return false; // Blok içindeki hiçbir statement sonlandırmıyorsa, blok da sonlandırmaz
    }

    // If/Switch/Match gibi kontrol yapıları daha karmaşıktır, tüm dallar sonlandırıyorsa blok sonlandırır.
    // Basitlik için diğer tüm statementların yürütmeyi devam ettirdiğini varsayalım.
    return false;
}


// AST'yi kontrol akışı analizi için dolaşan yardımcı fonksiyon (recursive)
// analyzeControlFlow'dan çağrılır ve iç içe ifadeleri ve alt statement'ları analiz eder.
// Erişim durumu bu dolaşma sırasında alt düğümlere iletilebilir.
int traverseASTForControlFlow(ASTNode* node, ReachabilityStatus inheritedStatus) {
     if (!node) return 0;
     int errors = 0;

    // Bu düğümün türüne göre özel kontrol akışı mantığı varsa burada ele alınır.
    // Örneğin, bir mantıksal AND (&&) veya OR (||) ifadesi kısa devre yapabilir ve kontrol akışını etkileyebilir.
     if (node.kind == SyntaxKind.BinaryOperatorExpressionNode) {
        auto binOp = cast(BinaryOperatorExpressionNode*)node;
        if (binOp.operatorToken.kind == SyntaxKind.AmpersandAmpersandToken) {
            // Sol taraf false ise sağ tarafa ulaşılamaz.
            errors += traverseASTForControlFlow(binOp.left, inheritedStatus);
            errors += traverseASTForControlFlow(binOp.right, ReachabilityStatus.MightBeReachable); // Sağ taraf koşullu erişilebilir
            return errors; // Çocukları manuel dolaştık, tekrar dolaşmaya gerek yok
        }
        // ... Diğer kısa devre yapan operatörler
     }


     // Varsayılan: Çocukları aynı erişilebilirlik durumuyla dolaş.
     foreach (child; node.children) {
         errors += traverseASTForControlFlow(child, inheritedStatus);
     }

     return errors;
}

// Kontrol Akışı Grafiği (CFG) Yapıları (Daha gelişmiş implementasyonlar için)

// Bir temel bloğu temsil eden yapı
struct BasicBlock {
    int id;             // Bloğun benzersiz ID'si
    ASTNode[] statements; // Bu bloktaki statement'lar (veya IR komutları)
    // Bu bloğun sonundaki kontrol akışı (örneğin, koşullu veya koşulsuz atlama)
     Edge[] outgoingEdges; // Çıkış kenarları
}

// CFG'deki bir kenarı (kontrol akışı yolu) temsil eden yapı
struct Edge {
    BasicBlock* targetBlock; // Gidilen hedef blok
    // Bu kenarın temsil ettiği koşul (koşullu dallanma için)
     ASTNode* condition;
}

// Bir fonksiyonun veya programın CFG'sini temsil eden sınıf
class ControlFlowGraph {
    BasicBlock[] blocks; // Tüm temel bloklar
    BasicBlock* entryBlock; // Giriş bloğu
    BasicBlock* exitBlock; // Çıkış bloğu (isteğe bağlı)

    // AST'den veya IR'dan CFG oluşturan fonksiyon
      static ControlFlowGraph buildFromAST(ASTNode* functionBody);
}

// CFG üzerinde veri akışı analizi yapan fonksiyonlar
// (Ödünç alma kontrolü gibi analizler için gereklidir)
 void analyzeDataFlow(ControlFlowGraph cfg);
