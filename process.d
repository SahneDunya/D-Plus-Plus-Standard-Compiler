module process;

import std.stdio;     // writeln için
import std.string;    // string işlemleri için
import std.array;     // Dizi işlemleri için
import core.result;   // Result<T, E> için
import core.option;   // Option<T> için
import core.error;    // Temel hata tipleri için
import sync;          // SyncError için (ChildProcess.wait)
// D'nin std.process modülü veya C process/exec API'leri kullanılabilir.
import std.process;  // D'nin std.process modülünü kullanalım
import core.time;     // Zaman aşımı (timeout) için

// Süreç işlemleri için hata türü (core.error.Error'dan türetilmiş)
struct ProcessError : Error {
    enum Kind {
        CommandNotFound,  // Çalıştırılacak komut bulunamadı
        ExecutionError,   // Komut çalıştırılırken hata
        WaitError,        // Alt sürece katılırken hata
        OtherError,       // Diğer hatalar
    }

    Kind kind;
    string message;

    this(Kind kind, string message = "") {
        this.kind = kind;
        this.message = message;
    }

    override string description() const {
        return format("ProcessError(%s): %s", kind.to!string, message.empty ? kind.to!string : message);
    }

    string toString() const {
        return description();
    }
}

string to!string(ProcessError.Kind kind) {
    final switch (kind) {
        case ProcessError.Kind.CommandNotFound: return "CommandNotFound";
        case ProcessError.Kind.ExecutionError: return "ExecutionError";
        case ProcessError.Kind.WaitError: return "WaitError";
        case ProcessError.Kind.OtherError: return "OtherError";
    }
}


// Çalışan bir alt süreci (child process) temsil eden yapı
class ChildProcess {
    // Altında yatan sistem alt süreç tanıtıcısı veya nesnesi
    private std.process.Process delegateProcess; // D'nin std.process.Process yapısı

    // Constructor (sadece command fonksiyonu tarafından çağrılmalı)
    private this(std.process.Process delegateProcess) {
        this.delegateProcess = delegateProcess;
    }

    // Alt sürecin tamamlanmasını bekler ve çıkış kodunu döndürür.
    // Başarılı durumda Ok(çıkış_kodu), hata durumunda Err(ProcessError) döndürür.
    Result!(int, ProcessError) wait() {
        if (!delegateProcess) {
            return Result!(int, ProcessError).Err(ProcessError(ProcessError.Kind.WaitError, "Geçersiz alt süreç."));
        }
        writeln("Alt süreç bekleniyor...");
        try {
            // D'nin Process.wait metodu kullanılabilir.
            int exitCode = delegateProcess.wait();
            writeln("Alt süreç tamamlandı. Çıkış kodu: ", exitCode);
            return Result!(int, ProcessError).Ok(exitCode);
        } catch (Exception e) {
            // Bekleme hatasını ProcessError'a çevir.
            stderr.writeln("Hata (Process): Alt süreç beklenirken hata: ", e.msg);
            return Result!(int, ProcessError).Err(ProcessError(ProcessError.Kind.WaitError, e.msg));
        }
    }

    // Alt sürece sinyal göndererek sonlandırmaya çalışır (kill).
    
    Result!(void, ProcessError) kill() {
        if (!delegateProcess) { ... }
        try {
            // D'nin Process.kill metodu kullanılabilir.
            delegateProcess.kill();
            return Result!(void, ProcessError).Ok(void);
        } catch (Exception e) { ... hata ... }
    }

    // Alt sürecin standart girdi/çıktı/hata akışlarına erişim (Pipe olarak)
    
    Result!(io.Write*, ProcessError) stdin() const; // io.d'deki Write traitini döndürür
    Result!(io.Read*, ProcessError) stdout() const;  // io.d'deki Read traitini döndürür
    Result!(io.Read*, ProcessError) stderr() const; // io.d'deki Read traitini döndürür

    // Manuel bellek yönetimi için destructor (Eğer ChildProcess class ve GC kullanmıyorsak)
    // Alt süreç hala çalışıyorsa ne yapmalı? (Beklemeli mi, sonlandırmalı mı?)
}


// Mevcut sürecin ID'sini döndürür
long id() {
    // D'nin std.process.pid() fonksiyonu kullanılabilir.
    return std.process.pid();
}

// Mevcut süreci belirli bir çıkış koduyla sonlandırır
void exit(int code) {
    // D'nin std.process.exit() fonksiyonu kullanılabilir.
    std.process.exit(code);
}

// Yeni bir süreci başlatmak için bir CommandBuilder oluşturur.
// Builder deseni, komut, argümanlar, çalışma dizini, ortam değişkenleri, I/O yönlendirmesi gibi seçenekleri yapılandırmak için yaygındır.
CommandBuilder command(string program) {
    return CommandBuilder(program); // Yeni builder nesnesi oluştur
}

// Komut başlatma seçeneklerini yapılandıran builder yapısı
struct CommandBuilder {
    private string program;     // Çalıştırılacak program
    private string[] arguments; // Program argümanları
    private string workingDirectory; // Çalışma dizini (isteğe bağlı)
    private string[string] environment; // Ortam değişkenleri (isteğe bağlı, null ise kalıtılır)
    // I/O yönlendirme seçenekleri (Pipe, Inherit, RedirectToFile)
    StandardInputOption stdinOption;
    StandardOutputOption stdoutOption;
    StandardErrorOption stderrOption;


    this(string program) {
        this.program = program;
        this.arguments = [];
        this.workingDirectory = ""; // Varsayılan: Mevcut çalışma dizini
        this.environment = null; // Varsayılan: Ortamı kalıt
        // Varsayılan I/O yönlendirmesi: Devral (Inherit)
    }

    // Argüman ekler
    CommandBuilder args(string[] args) {
        this.arguments ~= args;
        return this; // Builder'ı döndür zincirleme çağrı için
    }

     // Çalışma dizinini ayarlar
     CommandBuilder workingDir(string dir) {
        this.workingDirectory = dir;
        return this;
     }

    // Ortam değişkenlerini ayarlar
    CommandBuilder env(string[string] env) {
        this.environment = env;
        return this;
    }

    // ... I/O yönlendirme metotları (stdin, stdout, stderr)

    // Süreci başlatır ve ChildProcess nesnesini döndürür.
    Result!(ChildProcess*, ProcessError) spawn() const {
        writeln("Süreç başlatılıyor: ", program, " ", arguments.join(" "));
        try {
            // D'nin std.process.spawnProcess veya similar fonksiyonunu kullan
            // Program, argümanlar, çalışma dizini, ortam, I/O seçenekleri gibi bilgileri D'nin fonksiyonuna geçir.
            std.process.Process delegateProcess = std.process.spawnProcess(
                [program] ~ arguments, // Komut ve argümanları tek diziye birleştir
                workingDirectory.empty ? null : workingDirectory, // Çalışma dizini (boşsa null)
                environment.empty ? null : environment // Ortam (boşsa null)
                // I/O seçenekleri buraya eklenecek
            );
            writeln("Süreç başlatıldı. PID: ", delegateProcess.pid); // D'nin process ID'si
            return Result!(ChildProcess*, ProcessError).Ok(new ChildProcess(delegateProcess));
        } catch (ProcessException e) {
            // D'nin ProcessException'ını ProcessError'a çevir
            stderr.writeln("Hata (Process): Süreç başlatma hatası '", program, "': ", e.msg);
            // Hata türünü daha detaylı belirlemeye çalış (FileNotFound, PermissionDenied gibi)
            if (e.msg.contains("command not found")) return Result!(ChildProcess*, ProcessError).Err(ProcessError(ProcessError.Kind.CommandNotFound, e.msg));
            return Result!(ChildProcess*, ProcessError).Err(ProcessError(ProcessError.Kind.ExecutionError, e.msg));
        } catch (Exception e) {
            stderr.writeln("Hata (Process): Süreç başlatılırken beklenmeyen hata '", program, "': ", e.msg);
            return Result!(ChildProcess*, ProcessError).Err(ProcessError(ProcessError.Kind.OtherError, e.msg));
        }
    }
}