#include "CompilerDriver.h"

// LLVM Başlatma fonksiyonları implementasyonu
// Bu fonksiyonlar genellikle uygulamanın başlangıcında bir kez çağrılmalıdır.
// main fonksiyonunda veya burada çağırabilirsiniz.
// CodeGen constructor'ında da çağrılıyorlar, ancak burada çağırmak daha temizdir.
// Ancak çoklu başlatmayı önlemek için bir flag kontrolü eklemek gerekebilir.
 static bool llvmInitialized = false; if (!llvmInitialized) { ... init ...; llvmInitialized = true; }

// LLVM Başlatma Fonksiyonları
// TargetSelect başlığından gelir.
// Linkleme aşamasında çalıştırılmaları gerekir.
// Bu fonksiyonları bir kez ve uygulamanın başında çağırmak en iyisidir.
// İdeal olarak main.cpp'de olmalılar.
// Basitlik için şimdilik burada (constructor'da) çağıracağız, ancak dikkatli kullanılmalıdır.
void initializeLLVM() {
    // static bool initialized = false;
     if (!initialized) {
        LLVMInitializeAllTargetInfos();
        LLVMInitializeAllTargets();
        LLVMInitializeAllTargetMCs();
        LLVMInitializeAllAsmParsers();
        LLVMInitializeAllAsmPrinters();
         LLVMInitializeAllDisassemblers(); // İsteğe bağlı
         initialized = true;
     }
}


namespace dppc {

// CompilerDriver Constructor
CompilerDriver::CompilerDriver()
    : Reporter(std::cerr), // Hataları stderr'a yaz
      OutputKind(codegen::OutputType::Object), // Varsayılan çıktı object file
      OptimizationLevel(0) // Varsayılan optimizasyon seviyesi O0
{
    // LLVM'i başlat
    initializeLLVM();

    // Varsayılan hedef bilgilerini al
    getDefaultTarget();

     std::cerr << "DEBUG: CompilerDriver initialized." << std::endl; // Debug
}

// CompilerDriver Yıkıcı: Geçici dosyaları temizler
CompilerDriver::~CompilerDriver() {
    cleanupTemporaryFiles();
      std::cerr << "DEBUG: CompilerDriver destroyed, temporary files cleaned up." << std::endl; // Debug
}


// --- Komut Satırı Argümanlarını İşleme ---

bool CompilerDriver::parseCommandLineArgs(int argc, char* argv[]) {
    // Basit bir komut satırı ayrıştırma örneği
    // Gelişmiş bir parser kütüphanesi (getopt_long, boost::program_options vb.) kullanmak daha iyidir.

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o") {
            if (i + 1 < argc) {
                OutputFile = argv[++i];
            } else {
                Reporter.reportError(SourceLocation(), "-o seçeneği çıktı dosyası adı gerektirir.");
                return false;
            }
        } else if (arg == "--target") {
            if (i + 1 < argc) {
                TargetTriple = argv[++i];
                CPU = ""; // Hedef değiştiğinde CPU ve Features varsayılan olarak resetlenir
                Features = "";
            } else {
                Reporter.reportError(SourceLocation(), "--target seçeneği hedef üçlemesi gerektirir.");
                return false;
            }
        } else if (arg == "--cpu") {
            if (i + 1 < argc) {
                CPU = argv[++i];
            } else {
                Reporter.reportError(SourceLocation(), "--cpu seçeneği CPU adı gerektirir.");
                return false;
            }
        } else if (arg == "--features") {
            if (i + 1 < argc) {
                Features = argv[++i];
            } else {
                Reporter.reportError(SourceLocation(), "--features seçeneği özellik stringi gerektirir.");
                return false;
            }
        } else if (arg == "-O0") {
            OptimizationLevel = 0;
        } else if (arg == "-O1") {
            OptimizationLevel = 1;
        } else if (arg == "-O2") {
            OptimizationLevel = 2;
        } else if (arg == "-O3") {
            OptimizationLevel = 3;
        } else if (arg == "-emit-asm") {
            OutputKind = codegen::OutputType::Assembly;
             if (OutputFile.empty()) OutputFile = "output.s"; // Varsayılan .s uzantısı
        } else if (arg == "-emit-obj") {
             OutputKind = codegen::OutputType::Object;
             if (OutputFile.empty()) OutputFile = "a.o"; // Varsayılan .o uzantısı
        } else if (arg == "-c") { // Compile only, do not link
             OutputKind = codegen::OutputType::Object;
             // -c seçeneğiyle -o belirtilmezse, giriş dosyası adından .o türetilir.
             // Bu mantık run fonksiyonunda işlenebilir.
        } else if (arg[0] == '-') {
            Reporter.reportError(SourceLocation(), "Bilinmeyen komut satırı seçeneği: '" + arg + "'");
            return false;
        } else {
            // Giriş dosyası olarak kabul et
            InputFiles.push_back(arg);
        }
    }

    // En az bir giriş dosyası olmalı (executable çıktısı için)
    if (InputFiles.empty()) {
        Reporter.reportError(SourceLocation(), "Derlenecek giriş dosyası belirtilmedi.");
        // Tek dosya compile-only (-c veya emit-obj/asm) durumunda çıktı dosyası belirtilmediyse isim türetme logic'i
        // burada veya run fonksiyonunda olabilir.
         if (OutputKind != codegen::OutputType::Executable && InputFiles.size() == 1 && OutputFile.empty()) {
              // Tek giriş dosyası ve compile-only çıktı, çıktı adı türet.
              // Örn: input.d++ -> input.o
              std::string inputName = InputFiles[0];
              size_t lastDot = inputName.rfind('.');
              if (lastDot != std::string::npos) {
                  inputName = inputName.substr(0, lastDot);
              }
              OutputFile = inputName;
              if (OutputKind == codegen::OutputType::Object) OutputFile += ".o";
              else if (OutputKind == codegen::OutputType::Assembly) OutputFile += ".s";
         } else if (OutputKind == codegen::OutputType::Executable && OutputFile.empty()) {
             // Executable çıktısı ve çıktı adı belirtilmemişse varsayılan isim (örn: a.out)
             OutputFile = "a.out"; // Linux/macOS varsayılanı
             // Windows için "a.exe" olabilir. TargetTriple'a göre belirlenebilir.
         }

        // Giriş dosyası yoksa yine de hata
         Reporter.reportError(SourceLocation(), "Derlenecek giriş dosyası belirtilmedi.");
        return false;
    }

    // Eğer çıktı türü executable ve çıktı dosyası belirtilmediyse varsayılan isim atama (yukarı taşındı)


    return true;
}

// Varsayılan hedef bilgilerini alır
void CompilerDriver::getDefaultTarget() {
    // LLVM'in varsayılan hedef üçlemesini al (derleyicinin çalıştığı makineye göre)
    TargetTriple = llvm::sys::getDefaultTargetTriple();

    // Hedef üçlemesine göre varsayılan CPU ve özellikleri bul (opsiyonel)
    // Bu, TargetRegistry veya TargetMachine üzerinden yapılabilir.
    // Basitlik için şimdilik sadece üçlemeyi almakla yetinelim. CPU ve Features boş kalabilir.
     // TargetMachine::createTargetMachine'de varsayılanları kullanacaktır.
}

// Çıktı türünü string'den enum'a çevirir
codegen::OutputType CompilerDriver::stringToOutputType(const std::string& typeStr) {
    if (typeStr == "asm") return codegen::OutputType::Assembly;
    if (typeStr == "obj") return codegen::OutputType::Object;
    // Executable türü genellikle "-o" ve girdi dosyalarının varlığına göre belirlenir.
    // Bu metod sadece açık "-emit-asm" veya "-emit-obj" gibi seçenekler için kullanılabilir.
    // Diğer durumlarda varsayılan veya argüman parsing logic'i kullanılır.
    return codegen::OutputType::Object; // Varsayılan
}


// --- Derleme Aşamalarını Çalıştırma ---

// Lexer ve Parser'ı çalıştırır, AST üretir
std::unique_ptr<dppc::parser::ast::SourceFileAST> CompilerDriver::runLexerAndParser(const std::string& filename) {
    std::ifstream sourceFile(filename);
    if (!sourceFile.is_open()) {
        Reporter.reportError(SourceLocation(), "Giriş dosyası açılamadı: '" + filename + "'");
        return nullptr;
    }

    // Lexer'ı çalıştır
    dppc::lexer::Lexer lexer(sourceFile, Reporter);
    std::vector<dppc::lexer::Token> tokens = lexer.tokenize();

    if (Reporter.hasErrors()) {
        Reporter.reportMessage("Lexer aşamasında hatalar bulundu.", Reporter.getErrorCount(), MessageType::Error);
        return nullptr;
    }
      Reporter.reportMessage("Lexer aşaması başarılı.", Reporter.getErrorCount(), MessageType::Info); // Debug

    // Parser'ı çalıştır
    dppc::parser::Parser parser(tokens, Reporter);
    auto ast = parser.parseSourceFile();

    if (Reporter.hasErrors()) {
        Reporter.reportMessage("Parser aşamasında hatalar bulundu.", Reporter.getErrorCount(), MessageType::Error);
        return nullptr;
    }
      Reporter.reportMessage("Parser aşaması başarılı.", Reporter.getErrorCount(), MessageType::Info); // Debug

    return ast; // unique_ptr'ı döndür (sahipliği devret)
}

// Semantic Analyzer'ı çalıştırır
bool CompilerDriver::runSemanticAnalysis(dppc::parser::ast::SourceFileAST& ast, sema::SymbolTable& symbolTable, sema::TypeSystem& typeSystem) {
    sema::SemanticAnalyzer semanticAnalyzer(Reporter, symbolTable, typeSystem);
    bool success = semanticAnalyzer.analyze(ast);

    if (!success) {
        Reporter.reportMessage("Semantik Analiz aşamasında hatalar bulundu.", Reporter.getErrorCount(), MessageType::Error);
        return false;
    }
      Reporter.reportMessage("Semantik Analiz aşaması başarılı.", Reporter.getErrorCount(), MessageType::Info); // Debug

    return true;
}

// IR Generator'ı çalıştırır
std::unique_ptr<llvm::Module> CompilerDriver::runIRGeneration(dppc::parser::ast::SourceFileAST& ast, sema::SymbolTable& symbolTable, sema::TypeSystem& typeSystem, const std::string& moduleName) {
    irgen::IRGenerator irGenerator(Reporter, symbolTable, typeSystem, moduleName, TargetTriple);
    auto module = irGenerator.generate(ast);

    if (!module) {
        Reporter.reportMessage("IR Üretim aşamasında hatalar bulundu.", Reporter.getErrorCount(), MessageType::Error);
        return nullptr;
    }
      Reporter.reportMessage("IR Üretim aşaması başarılı.", Reporter.getErrorCount(), MessageType::Info); // Debug

    // TODO: Optimizasyon aşaması burada çalıştırılabilir.
    // LLVM optimizasyon pass'lerini çalıştırmak (PassManagerBuilder vb.)

    return module; // unique_ptr'ı döndür
}

// Code Generator'ı çalıştırır
bool CompilerDriver::runCodeGeneration(llvm::Module& module, const std::string& outputFilename) {
     // CodeGen nesnesi constructor'da hedef makineyi oluşturur.
    codegen::CodeGen codeGen(Reporter, TargetTriple, CPU, Features, OutputKind);

    bool success = codeGen.generate(module, outputFilename);

    if (!success) {
        Reporter.reportMessage("Kod Üretim aşamasında hatalar bulundu.", Reporter.getErrorCount(), MessageType::Error);
        return false;
    }
      Reporter.reportMessage("Kod Üretim aşaması başarılı.", Reporter.getErrorCount(), MessageType::Info); // Debug

    return true;
}

// Linker'ı çalıştırır (executable çıktısı için)
bool CompilerDriver::runLinker(const std::vector<std::string>& objectFiles, const std::string& executableName) {
    if (objectFiles.empty()) {
        Reporter.reportError(SourceLocation(), "Linkleme için object dosyası bulunamadı.");
        return false;
    }

    // Sistem linker'ını çağır (örn: g++, clang++, ld)
    // Linker komut satırını oluştur
    std::string linkerCommand = "g++"; // Varsayılan olarak g++ kullanalım (C++ runtime kütüphaneleri için)
    // TargetTriple'a göre cross-linker seçimi yapılabilir.
      if (TargetTriple.find("riscv64") != std::string::npos) linkerCommand = "riscv64-unknown-elf-g++"; // Örnek cross-linker

    for (const auto& objFile : objectFiles) {
        linkerCommand += " " + objFile;
    }
    linkerCommand += " -o " + executableName;

    // TODO: Gerekli kütüphaneleri ekle (-ld++runtime, -lm vb.)
     linkerCommand += " -L/path/to/d++runtime -ld++runtime";


    Reporter.reportMessage("Linker komutu çalıştırılıyor: " + linkerCommand, 0, MessageType::Info); // Debug

    // Komutu sistemde çalıştır
    int linkerResult = std::system(linkerCommand.c_str());

    if (linkerResult != 0) {
        Reporter.reportError(SourceLocation(), "Linkleme başarısız oldu (çıkış kodu: " + std::to_string(linkerResult) + ").");
        // Linker hataları genellikle stderr'a yazılır, onları yakalamak daha karmaşıktır.
        return false;
    }

     Reporter.reportMessage("Linkleme başarılı. Çıktı: " + executableName, 0, MessageType::Info); // Debug

    return true;
}

// Geçici dosyaları takip etme
void CompilerDriver::addTemporaryFile(const std::string& filepath) {
     TemporaryFiles.push_back(filepath);
}

void CompilerDriver::cleanupTemporaryFiles() {
    for (const auto& filepath : TemporaryFiles) {
        if (std::remove(filepath.c_str()) == 0) {
              std::cerr << "DEBUG: Geçici dosya silindi: " << filepath << std::endl; // Debug
        } else {
              std::cerr << "DEBUG: Geçici dosya silinemedi: " << filepath << std::endl; // Debug
        }
    }
    TemporaryFiles.clear();
}


// --- Ana Derleme Akışı ---

int CompilerDriver::run(int argc, char* argv[]) {
    // Hata sayısını sıfırla (her çalıştırmada)
    Reporter.reset();
    TemporaryFiles.clear(); // Önceki çalıştırmadan kalma temp dosyaları temizle

    // Komut satırı argümanlarını ayrıştır
    if (!parseCommandLineArgs(argc, argv)) {
        Reporter.reportSummary(); // Ayrıştırma hatası varsa raporla
        return 1; // Hata kodu
    }

    std::vector<std::string> generatedObjectFiles; // Linklenecek object dosyaları

    // Tüm giriş dosyalarını ayrı ayrı derle
    // Birden fazla giriş dosyası varsa ve çıktı executable ise, her dosya için object file üretilir
    // ve sonra bunlar linklenir.
    // Eğer çıktı assembly veya object ise ve birden fazla giriş dosyası varsa, bu bir hata olabilir
    // veya her dosya için ayrı çıktı dosyası üretilebilir (bu örnekte tek çıktı dosyası varsayılıyor).
    if ((OutputKind == codegen::OutputType::Assembly || OutputKind == codegen::OutputType::Object) && InputFiles.size() > 1 && OutputFile != "-") { // "-" stdout için
        Reporter.reportError(SourceLocation(), "Assembly veya object çıktı türü için sadece tek bir giriş dosyası destekleniyor veya çıktı stdout olmalı ('-o -').");
        Reporter.reportSummary();
        return 1;
    }


    // Her giriş dosyası için derleme pipeline'ını çalıştır
    for (const auto& inputFile : InputFiles) {
        Reporter.reportMessage("Derleniyor: " + inputFile, 0, MessageType::Info); // Debug

        // Her dosya için yeni bir SymbolTable ve TypeSystem (veya paylaşılabilir)
        // Genellikle SymbolTable dosya bazlıdır (compilation unit). TypeSystem ise tüm proje için aynıdır.
        sema::TypeSystem typeSystem; // TypeSystem tüm modüller için aynı olmalı. Driver'da tutulup pass edilebilir.
        sema::SymbolTable symbolTable; // SymbolTable her modül için ayrıdır.

        // Lexer ve Parser
        auto ast = runLexerAndParser(inputFile);
        if (!ast) {
            // Hata raporlandı, sonraki aşamalara geçme
            continue; // Bir sonraki giriş dosyasına geç
        }

        // Semantic Analysis
        if (!runSemanticAnalysis(*ast, symbolTable, typeSystem)) {
            // Hata raporlandı
            continue; // Bir sonraki giriş dosyasına geç
        }

        // IR Generation
        // Modül adı genellikle giriş dosyasının adıdır.
        std::string moduleName = inputFile; // Dosya adını modül adı olarak kullan
        auto llvmModule = runIRGeneration(*ast, symbolTable, typeSystem, moduleName);
        if (!llvmModule) {
             // Hata raporlandı
             continue; // Bir sonraki giriş dosyasına geç
        }


        // Code Generation (Assembly veya Object)
        std::string currentOutputFilename;
        if (OutputKind == codegen::OutputType::Executable) {
            // Executable çıktısı isteniyorsa, her dosya için geçici bir object file üret
            // Örn: input.d++ -> input.o.tmp
            std::string baseName = inputFile;
            size_t lastDot = baseName.rfind('.');
            if (lastDot != std::string::npos) {
                baseName = baseName.substr(0, lastDot);
            }
            currentOutputFilename = baseName + ".o.tmp";
            addTemporaryFile(currentOutputFilename); // Temizlenecek dosyalar listesine ekle
            // CodeGen çıktısı artık object file olacak
            OutputKind = codegen::OutputType::Object; // Geçici olarak object olarak ayarla
        } else {
             // Tek dosya çıktısı (assembly veya object) veya stdout
             currentOutputFilename = OutputFile;
        }

        // Code Generator'ı çalıştır
        if (runCodeGeneration(*llvmModule, currentOutputFilename)) {
            // Kod üretimi başarılıysa
            if (OutputKind == codegen::OutputType::Object) {
                 // Object file üretildiyse, linkleme listesine ekle (executable çıktısı için)
                 if (this->OutputKind == codegen::OutputType::Executable) { // Orijinal OutputKind'a bak
                     generatedObjectFiles.push_back(currentOutputFilename);
                 }
                 // Eğer orijinal çıktı türü zaten Object idi, işlem tamam.
            }
            // Assembly çıktısı durumunda işlem tamam.

        } else {
            // Kod üretimi başarısız oldu
            continue; // Bir sonraki giriş dosyasına geç
        }

         // Eğer çıktı türü executable değilse, her dosya ayrı bir çıktı üretir.
         // İlk dosya dışındakiler için çıktı dosyasını belirtmek veya her dosya için ayrı ayrı isimlendirmek
         // kullanıcı arayüzü tasarımına bağlıdır. Şu anki mantıkta tek çıktı dosyası varsayılıyor
         // veya executable çıktısı için her dosya tmp .o üretir.

         // Reset OutputKind for the next file if it was temporarily set to Object for executable linking
        if (this->OutputKind == codegen::OutputType::Executable) {
             // Do nothing, it was already set to Executable originally
        } else {
             // If it was Assembly or Object, and there are more input files, this is a design issue.
             // Current logic expects single output for Assembly/Object unless stdout.
        }
    }

    // Linkleme aşaması (sadece executable çıktısı isteniyorsa)
    if (this->OutputKind == codegen::OutputType::Executable) {
        Reporter.reportMessage("Linkleme başlatılıyor.", 0, MessageType::Info); // Debug
        if (!runLinker(generatedObjectFiles, OutputFile)) {
            // Linkleme başarısız oldu
             Reporter.reportSummary();
             return 1; // Hata kodu
        }
    }

    // Tüm aşamalar tamamlandı (veya hatalarla atlandı), hata özetini raporla
    Reporter.reportSummary();

    // Hata var mı kontrol et
    if (Reporter.hasErrors()) {
        return 1; // Hata kodu
    }

    return 0; // Başarı kodu
}

} // namespace dppc
