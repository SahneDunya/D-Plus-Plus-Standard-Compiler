module compiler;

import std.stdio;
import std.file;
import frontend; // frontend.d dosyasını import ediyoruz
import semantics; // İleride semantic.d dosyasını import edeceksiniz
import backend;   // İleride backend.d dosyasını import edeceksiniz
import ir;        // İleride ir.d dosyasını import edeceksiniz
import error_reporting; // İleride hata raporlama mekanizmasını import edeceksiniz

// Derleyici yapılandırması veya seçenekleri için bir struct olabilir
struct CompilerOptions {
    string inputFile;
    string outputFile;
    // Diğer seçenekler: optimizasyon seviyesi, hedef platform vb.
}

// Ana derleme fonksiyonu
int compile(CompilerOptions options) {
    // 1. Kaynak kodu oku
    string sourceCode;
    try {
        sourceCode = readText(options.inputFile);
        writeln("Kaynak dosya okundu: ", options.inputFile);
    } catch (Exception e) {
        // error_reporting.reportError("Dosya okuma hatası: ", e.msg); // İleride kullanılabilir
        stderr.writeln("Hata: Kaynak dosya okunamadı '", options.inputFile, "': ", e.msg);
        return 1; // Hata kodu
    }

    // 2. Frontend Aşaması: Tarama ve Ayrıştırma
    writeln("Frontend aşaması başlatılıyor...");
    ASTNode* syntaxTree = null; // Soyut Sözdizimi Ağacı
    Token[] tokens; // Belirteçler listesi

    try {
        tokens = lex(sourceCode); // frontend.d'deki lex fonksiyonunu çağır
        writeln("Tarama tamamlandı. Token sayısı: ", tokens.length);

        syntaxTree = parse(tokens); // frontend.d'deki parse fonksiyonunu çağır
        writeln("Ayrıştırma tamamlandı. AST oluşturuldu.");

        // Basitçe AST'nin türünü yazdıralım (ileride daha detaylı olabilir)
         if (syntaxTree) {
             writeln("Oluşturulan AST'nin kök düğüm türü: ", syntaxTree.kind); // Varsayım: ASTNode struct'ında 'kind' alanı var
         }

    } catch (Exception e) {
        // error_reporting.reportError("Frontend hatası: ", e.msg); // İleride kullanılabilir
        stderr.writeln("Hata: Frontend aşamasında bir hata oluştu: ", e.msg);
        // Eğer AST oluştuysa temizleme gerekebilir
         destroyAST(syntaxTree); // İleride yazılacak bir fonksiyon
        return 1; // Hata kodu
    }

    // Eğer ayrıştırma başarılıysa devam et
    if (!syntaxTree) {
         stderr.writeln("Hata: Ayrıştırma sonucu AST oluşturulamadı.");
         return 1;
    }


    // 3. Semantik Analiz (İleride implement edilecek)
     writeln("Semantik analiz aşaması başlatılıyor...");
     bool semanticErrorsFound = analyzeSemantics(syntaxTree); // semantics.d'deki fonksiyon
     if (semanticErrorsFound) {
         stderr.writeln("Hata: Semantik hatalar bulundu. Derleme durduruldu.");
          destroyAST(syntaxTree);
         return 1;
     }
     writeln("Semantik analiz tamamlandı. Hata bulunamadı.");


    // 4. Kod Üretimi (İleride implement edilecek)
     writeln("Kod üretimi aşaması başlatılıyor...");
     int backendResult = generateCode(syntaxTree, options.outputFile); // backend.d'deki fonksiyon
        destroyAST(syntaxTree); // AST artık gerekli değilse temizle
     if (backendResult != 0) {
         stderr.writeln("Hata: Kod üretim aşamasında bir hata oluştu.");
         return 1;
     }
     writeln("Kod üretimi tamamlandı: ", options.outputFile);


    writeln("Derleme başarıyla tamamlandı.");
    return 0; // Başarılı kod
}

int main(string[] args) {
    if (args.length < 2) {
        stderr.writeln("Kullanım: d++ <kaynak_dosya>");
        return 1;
    }

    CompilerOptions options;
    options.inputFile = args[1];
    options.outputFile = "a.out"; // Varsayılan çıktı dosyası

    return compile(options);
}