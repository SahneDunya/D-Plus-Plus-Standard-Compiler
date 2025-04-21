module error_reporting;

import std.stdio;
import std.string;
import std.format;

// Tanısal mesajın seviyeleri
enum DiagnosticLevel {
    Info,    // Bilgilendirme mesajı
    Warning, // Uyarı (derlemeyi durdurmaz, potansiyel sorun)
    Error,   // Hata (derlemeyi durdurur)
    Fatal    // Kritik Hata (acil durdurma gerektiren durum)
}

// Tanısal mesajı (hata, uyarı vb.) temsil eden yapı
struct Diagnostic {
    DiagnosticLevel level;  // Mesajın seviyesi
    string message;         // Mesajın içeriği
    string filePath;        // Hatanın/uyarının oluştuğu dosya yolu
    int lineNumber;         // Hatanın/uyarının oluştuğu satır numarası
    int columnNumber;       // Hatanın/uyarının oluştuğu sütun numarası

    // İlgili kod parçasını da tutabilir
     string sourceLine;
     int startColumn; // İşaretlenmesi gereken kısmın başlangıç sütunu
     int endColumn;   // İşaretlenmesi gereken kısmın bitiş sütunu
}

// Toplanan tanısal mesajları tutan merkezi liste
private Diagnostic[] diagnostics;

// Belirli bir seviyede bir tanısal mesaj raporlar
void reportDiagnostic(Diagnostic diagnostic) {
    diagnostics ~= diagnostic; // Mesajı listeye ekle
    printDiagnostic(diagnostic); // Anında konsola yazdır
}

// Bilgilendirme mesajı raporlar
void reportInfo(string message, string filePath = "", int lineNumber = 0, int columnNumber = 0) {
    reportDiagnostic(Diagnostic(DiagnosticLevel.Info, message, filePath, lineNumber, columnNumber));
}

// Uyarı mesajı raporlar
void reportWarning(string message, string filePath = "", int lineNumber = 0, int columnNumber = 0) {
    reportDiagnostic(Diagnostic(DiagnosticLevel.Warning, message, filePath, lineNumber, columnNumber));
}

// Hata mesajı raporlar
void reportError(string message, string filePath = "", int lineNumber = 0, int columnNumber = 0) {
    reportDiagnostic(Diagnostic(DiagnosticLevel.Error, message, filePath, lineNumber, columnNumber));
}

// Kritik hata mesajı raporlar (Genellikle programın sonlanmasına yol açar)
void reportFatalError(string message, string filePath = "", int lineNumber = 0, int columnNumber = 0) {
    reportDiagnostic(Diagnostic(DiagnosticLevel.Fatal, message, filePath, lineNumber, columnNumber));
    // Fatal hatalarda programı durdurmak isteyebilirsiniz.
    import std.process;
     exit(1); // Hata kodu ile çıkış yap
}


// Toplanan tüm tanısal mesajları konsola yazdırır
// Genellikle derleme sonunda özet raporu için kullanılır.
void printDiagnosticsSummary() {
    writeln("\n--- Tanısal Mesaj Özeti ---");
    int errorCount = 0;
    int warningCount = 0;
    int infoCount = 0;

    foreach (d; diagnostics) {
        final switch (d.level) {
            case DiagnosticLevel.Error: errorCount++; break;
            case DiagnosticLevel.Fatal: errorCount++; break; // Fatal hataları da toplam hata sayısına dahil et
            case DiagnosticLevel.Warning: warningCount++; break;
            case DiagnosticLevel.Info: infoCount++; break;
        }
    }

    writeln("Toplam ", errorCount, " Hata, ", warningCount, " Uyarı, ", infoCount, " Bilgi.");
    writeln("-------------------------");
}

// Tek bir tanısal mesajı formatlayıp konsola yazdırır
private void printDiagnostic(Diagnostic d) {
    // Hata/uyarı seviyesini renkli yazdırabilirsiniz
    string levelStr;
    mixin(switch(d.level) {
        case DiagnosticLevel.Info: levelStr = "Bilgi"; break;
        case DiagnosticLevel.Warning: levelStr = "\033[1;33mUyarı\033[0m"; break; // Sarı
        case DiagnosticLevel.Error: levelStr = "\033[1;31mHata\033[0m"; break;   // Kırmızı
        case DiagnosticLevel.Fatal: levelStr = "\033[1;31mKRİTİK HATA\033[0m"; break; // Kırmızı
    });

    stderr.writef("[%s] ", levelStr);

    // Konum bilgisini yazdır (varsa)
    if (!d.filePath.empty) {
        stderr.writef("%s:%s:%s: ", d.filePath, d.lineNumber, d.columnNumber);
    }

    // Mesajı yazdır
    stderr.writeln(d.message);

    // Eğer ilgili kod satırı bilgisi varsa, kodu ve hata konumunu işaretleyerek yazdırabilirsiniz.
    
    if (!d.sourceLine.empty) {
        stderr.writeln(d.sourceLine);
        // Hata konumunu işaretlemek için '^' karakterleri kullan
        string pointerLine = format("%*s", d.startColumn > 0 ? d.startColumn - 1 : 0, "");
        int pointerWidth = d.endColumn >= d.startColumn ? d.endColumn - d.startColumn : 1;
        pointerLine ~= format("%s", std.string.replicate('^', pointerWidth));
        stderr.writeln(pointerLine);
    }
    
}

// Toplanan tüm tanısal mesajları temizler (Yeni bir derleme başlamadan önce)
void clearDiagnostics() {
    diagnostics.length = 0;
}

// Derlemenin başarılı olup olmadığını kontrol etmek için (Hata yoksa başarılı)
bool compilationSuccessful() {
    foreach (d; diagnostics) {
        if (d.level == DiagnosticLevel.Error || d.level == DiagnosticLevel.Fatal) {
            return false;
        }
    }
    return true;
}