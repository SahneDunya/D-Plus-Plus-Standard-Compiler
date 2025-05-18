#include "IRGenerator.h"
#include "llvm/IR/Verifier.h" // verifyModule için
#include "llvm/Support/Host.h" // getDefaultTargetTriple için
#include "llvm/ADT/StringRef.h" // StringRef için
#include "llvm/IR/Type.h" // LLVM Type static metotları için

#include <iostream> // Debug için

namespace dppc {
namespace irgen {

// IRGenerator Constructor
IRGenerator::IRGenerator(ErrorReporter& reporter, SymbolTable& symbols, TypeSystem& types, const std::string& moduleName, const std::string& targetTriple)
    : Reporter(reporter), Symbols(symbols), Types(types),
      Context(std::make_unique<llvm::LLVMContext>()),
      Module(std::make_unique<llvm::Module>(moduleName, *Context)),
      Builder(std::make_unique<llvm::IRBuilder<>>(*Context))
{
    // Modülün hedef mimarisini ve veri düzenini ayarla
    // TargetTriple main.cpp'den gelmelidir (örn: "riscv64-unknown-elf")
    Module->setTargetTriple(targetTriple);
    // DataLayout'u belirlemek daha karmaşıktır ve hedef mimariye bağlıdır.
    // Genellikle bir TargetMachine nesnesinden alınır, veya varsayılan bir layout ayarlanır.
    // Şimdilik boş bırakalım veya basit bir varsayılan deneyelim.
     Module->setDataLayout("e-m:e-p:64:64-i64:64-i128:128-n64-S128"); // Örnek RISC-V 64-bit layout


    // Ön tanımlı D++ tiplerini LLVM tiplerine çevir ve map'e kaydet
    DppToLLVMTypes[Types.getErrorType()] = llvm::Type::getVoidTy(*Context); // Hata tipi için void veya özel bir LLVM tipi
    DppToLLVMTypes[Types.getVoidType()] = llvm::Type::getVoidTy(*Context);
    DppToLLVMTypes[Types.getBooleanType()] = llvm::Type::getInt1Ty(*Context); // Bool için 1-bit integer
    DppToLLVMTypes[Types.getIntType()] = llvm::Type::getInt32Ty(*Context); // Varsayılan int boyutu 32-bit
    DppToLLVMTypes[Types.getFloatType()] = llvm::Type::getFloatTy(*Context); // Varsayılan float (32-bit) veya DoubleTy (64-bit)
    DppToLLVMTypes[Types.getStringType()] = llvm::PointerType::get(llvm::Type::getInt8Ty(*Context), 0); // String için char* (C-stili)
    DppToLLVMTypes[Types.getCharacterType()] = llvm::Type::getInt8Ty(*Context); // Char için 8-bit integer
    // TODO: Diğer ilkel tipleri çevir (ubyte, short, long, float64, real vb.)
     DppToLLVMTypes[Types.getUbyteType()] = llvm::Type::getInt8Ty(*Context);
     DppToLLVMTypes[Types.getLongType()] = llvm::Type::getInt64Ty(*Context);
     DppToLLVMTypes[Types.getFloat64Type()] = llvm::Type::getDoubleTy(*Context);

     std::cerr << "DEBUG: IRGenerator initialized with target " << targetTriple << std::endl; // Debug
}


// D++ Tipini LLVM Tipine çevirir
llvm::Type* IRGenerator::generateType(sema::Type* dppType) {
    if (!dppType || dppType->isErrorType()) {
         // Hata tipini veya null pointer'ı çevirmeye çalışmak
         Reporter.reportError(SourceLocation(), "IR üretimi: Geçersiz veya hata tipi çevrilmeye çalışılıyor."); // Dahili hata
         return llvm::Type::getVoidTy(*Context); // Güvenli bir varsayılan
    }

    // Daha önce çevrilmiş mi kontrol et (map'i kullan)
    auto it = DppToLLVMTypes.find(dppType);
    if (it != DppToLLVMTypes.end()) {
        return it->second; // Var olan LLVM tipini döndür
    }

    // Yeni bir tip, çevir ve kaydet
    llvm::Type* llvmType = nullptr;

    switch (dppType->getKind()) {
        case sema::TypeKind::Void: llvmType = llvm::Type::getVoidTy(*Context); break;
        case sema::TypeKind::Boolean: llvmType = llvm::Type::getInt1Ty(*Context); break;
        case sema::TypeKind::Integer:
            // D++ Integer tipinin boyutuna göre LLVM integer tipini belirle
            llvmType = getLLVMIntegerType(dppType);
            break;
        case sema::TypeKind::FloatingPoint:
            // D++ Floating point tipinin boyutuna göre LLVM floating point tipini belirle
            llvmType = getLLVMFloatingPointType(dppType);
            break;
        case sema::TypeKind::String:
            llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(*Context), 0); // char*
            break;
        case sema::TypeKind::Character:
             llvmType = llvm::Type::getInt8Ty(*Context); // i8
             break;
        case sema::TypeKind::Pointer:
            if (auto ptrType = dynamic_cast<sema::PointerType*>(dppType)) {
                llvm::Type* baseLLVMType = generateType(ptrType->getBaseType()); // Alt tipi recursive çevir
                 if (baseLLVMType) llvmType = llvm::PointerType::get(baseLLVMType, 0);
            }
            break;
        case sema::TypeKind::Reference:
            if (auto refType = dynamic_cast<sema::ReferenceType*>(dppType)) {
                llvm::Type* baseLLVMType = generateType(refType->getBaseType()); // Alt tipi recursive çevir
                // Referanslar LLVM'de genellikle pointer olarak temsil edilir
                if (baseLLVMType) llvmType = llvm::PointerType::get(baseLLVMType, 0);
            }
            break;
        case sema::TypeKind::Array:
             if (auto arrayType = dynamic_cast<sema::ArrayType*>(dppType)) {
                 llvm::Type* elementLLVMType = generateType(arrayType->getElementType());
                 if (elementLLVMType) {
                     if (arrayType->getSize() >= 0) { // Sabit boyutlu dizi [T; N]
                         llvmType = llvm::ArrayType::get(elementLLVMType, arrayType->getSize());
                     } else { // Slice [T] (boyutu bilinmeyen dizi)
                         // Slice, genellikle bir pointer ve bir boyut (size) ile temsil edilir.
                         // { i8*, i64 } gibi bir struct olabilir.
                         // TODO: Slice temsilini belirle ve burada LLVM StructType oluştur.
                         Reporter.reportError(SourceLocation(), "IR üretimi: Slice tipi çevirisi henüz implement edilmedi.");
                         llvmType = llvm::Type::getVoidTy(*Context); // Placeholder
                     }
                 }
             }
             break;
        case sema::TypeKind::Tuple:
             if (auto tupleType = dynamic_cast<sema::TupleType*>(dppType)) {
                 std::vector<llvm::Type*> elementLLVMTypes;
                 bool error = false;
                 for (const auto& elemType : tupleType->getElementTypes()) {
                     if (llvm::Type* elemLLVMType = generateType(elemType)) {
                         elementLLVMTypes.push_back(elemLLVMType);
                     } else { error = true; break; }
                 }
                 if (!error) llvmType = llvm::StructType::get(*Context, elementLLVMTypes); // Tuple LLVM'de struct olarak temsil edilir
             }
             break;
        case sema::TypeKind::Function:
             if (auto funcType = dynamic_cast<sema::FunctionType*>(dppType)) {
                 llvm::Type* returnLLVMType = generateType(funcType->getReturnType());
                 std::vector<llvm::Type*> paramLLVMTypes;
                 bool error = false;
                 for (const auto& paramType : funcType->getParameterTypes()) {
                     if (llvm::Type* paramLLVMType = generateType(paramType)) {
                         paramLLVMTypes.push_back(paramLLVMType);
                     } else { error = true; break; }
                 }
                 if (!error && returnLLVMType) {
                     // LLVM fonksiyon tipleri geri dönüş tipi ve parametre tipleri alır
                     llvmType = llvm::FunctionType::get(returnLLVMType, paramLLVMTypes, false); // false: vararg değil
                 }
             }
             break;
        case sema::TypeKind::Struct:
            if (auto structType = dynamic_cast<sema::StructType*>(dppType)) {
                 // Struct tipleri tanımlanırken (NamedTypes'a eklenirken) burada oluşturulur.
                 // Eğer daha önce oluşturulmuşsa onu döndür, aksi halde yeni oluştur.
                 // Yapı adıyla LLVM StructType::create methodu kullanılabilir.
                 // Alanlar daha sonra StructType nesnesine set edilebilir.
                 // Şimdilik basit bir placeholder LLVM StructType oluşturalım.
                 // Tamamlama için, StructType::create ile adı verilen bir struct oluşturup
                 // analyzeStructDecl içinde alanları analiz ettikten sonra setBody metodunu çağırmanız gerekir.
                 llvmType = llvm::StructType::create(*Context, structType->getName()); // Adı verilen opak olmayan struct oluştur
                 // Tam alanları belirlemek için:
                  std::vector<llvm::Type*> fieldLLVMTypes;
                  for(const auto& field : structType->getFields()) { fieldLLVMTypes.push_back(generateType(field.type)); }
                  static_cast<llvm::StructType*>(llvmType)->setBody(fieldLLVMTypes);
                 Reporter.reportError(SourceLocation(), "IR üretimi: Struct tipi çevirisi alan detayları hariç yapılıyor."); // Geçici mesaj
            }
            break;
        case sema::TypeKind::Enum:
             if (auto enumType = dynamic_cast<sema::EnumType*>(dppType)) {
                 // Enumlar LLVM'de farklı şekillerde temsil edilebilir:
                 // - Basit integer (C enum gibi)
                 // - Tagged union (discriminant + en büyük varyantın verisi)
                 // Basitlik için C enum gibi integer temsilini varsayalım.
                 llvmType = llvm::Type::getInt32Ty(*Context); // Enum değerleri için int
                 Reporter.reportError(SourceLocation(), "IR üretimi: Enum tipi C-stili int olarak çevriliyor."); // Geçici mesaj
             }
             break;
        case sema::TypeKind::Trait:
             // Trait tipleri doğrudan IR'da temsil edilmez, TraitObject kullanılır.
             Reporter.reportError(SourceLocation(), "IR üretimi: Trait tipi doğrudan çevrilemez.");
             llvmType = llvm::Type::getVoidTy(*Context); // Hata veya placeholder
             break;
        case sema::TypeKind::TraitObject:
             // Trait Object (dyn Trait) genellikle bir data pointer'ı ve bir vtable pointer'ı ile temsil edilir.
             // { i8*, i8** } gibi bir struct olabilir.
             Reporter.reportError(SourceLocation(), "IR üretimi: TraitObject tipi çevirisi henüz implement edilmedi.");
             llvmType = llvm::Type::getVoidTy(*Context); // Placeholder
             break;
        case sema::TypeKind::Option:
             if (auto optionType = dynamic_cast<sema::OptionType*>(dppType)) {
                 llvm::Type* innerLLVMType = generateType(optionType->getInnerType());
                 if (innerLLVMType) {
                     // Option<T> genellikle { T, bool } veya { T*, bool } gibi bir struct olarak temsil edilir.
                     // bool Some/None durumunu belirler.
                     Reporter.reportError(SourceLocation(), "IR üretimi: Option<T> tipi çevirisi henüz implement edilmedi.");
                     llvmType = llvm::Type::getVoidTy(*Context); // Placeholder
                 }
             }
             break;
        case sema::TypeKind::Result:
             if (auto resultType = dynamic_cast<sema::ResultType*>(dppType)) {
                 llvm::Type* okLLVMType = generateType(resultType->getOkType());
                 llvm::Type* errLLVMType = generateType(resultType->getErrType());
                 if (okLLVMType && errLLVMType) {
                      // Result<T, E> genellikle { T veya E, bool } veya { T*, E*, bool } gibi bir struct olarak temsil edilir.
                      // bool Ok/Err durumunu belirler.
                      // Daha karmaşık tag+union yapısı olabilir.
                      Reporter.reportError(SourceLocation(), "IR üretimi: Result<T, E> tipi çevirisi henüz implement edilmedi.");
                      llvmType = llvm::Type::getVoidTy(*Context); // Placeholder
                 }
             }
             break;

        default:
            Reporter.reportError(SourceLocation(), "IR üretimi: Bilinmeyen D++ tip türü.");
            llvmType = llvm::Type::getVoidTy(*Context);
            break;
    }

    // Çevrilen LLVM tipini map'e kaydet
    if (llvmType) {
        DppToLLVMTypes[dppType] = llvmType;
    } else {
         // Çeviri başarısız olursa hata tipini kaydet veya hata raporla
          Reporter.reportError(SourceLocation(), "IR üretimi: Tip çevirisi başarısız.");
         DppToLLVMTypes[dppType] = llvm::Type::getVoidTy(*Context); // En azından bir şey kaydet
         llvmType = llvm::Type::getVoidTy(*Context);
    }

    return llvmType;
}


// Helper: D++ Integer tipinin boyutuna göre LLVM integer tipini al
llvm::Type* IRGenerator::getLLVMIntegerType(sema::Type* dppType) {
     // TypeSystem'da PrimitiveType sınıfına int boyut bilgisi eklenmelidir (örn: 8, 16, 32, 64).
     // Şimdilik varsayılan int boyutu 32-bit varsayalım.
      if (auto primType = dynamic_cast<sema::PrimitiveType*>(dppType)) {
         if (primType->isIntegerType()) { // isIntegerType kontrolü zaten yapıldı
             int bitWidth = primType->getBitWidth(); // PrimitiveType'da getBitWidth metodu olduğunu varsayalım
             return llvm::Type::getIntNType(*Context, bitWidth);
         }
      }
     Reporter.reportNote(SourceLocation(), "IR üretimi: Integer boyutu belirleme varsayılan (i32).");
     return llvm::Type::getInt32Ty(*Context); // Varsayılan
}

// Helper: D++ Floating point tipinin boyutuna göre LLVM floating point tipini al
llvm::Type* IRGenerator::getLLVMFloatingPointType(sema::Type* dppType) {
    // TypeSystem'da PrimitiveType sınıfına float boyutu bilgisi veya spesifik türü eklenmelidir (float32, float64).
    // Şimdilik varsayılan float boyutu 32-bit varsayalım.
     if (auto primType = dynamic_cast<sema::PrimitiveType*>(dppType)) {
         if (primType->isFloatingPointType()) {
             // float vs double vs real ayrımı burada yapılabilir.
             if (primType->getName() == "float") return llvm::Type::getFloatTy(*Context); // 32-bit IEEE 754
             if (primType->getName() == "double") return llvm::Type::getDoubleTy(*Context); // 64-bit IEEE 754
    //         // TODO: real ve diğer floating point tipleri
         }
     }
    Reporter.reportNote(SourceLocation(), "IR üretimi: Floating point boyutu belirleme varsayılan (float).");
    return llvm::Type::getFloatTy(*Context); // Varsayılan
}


// --- IR Üretim Fonksiyonları ---

// AST'nin tamamını IR'a çeviren ana fonksiyon
std::unique_ptr<llvm::Module> IRGenerator::generate(ast::SourceFileAST& root) {
    // Global scope'taki bildirimleri gez ve IR üret
    for (const auto& decl : root.Declarations) {
        generateDecl(decl.get()); // unique_ptr'dan raw pointer
    }

    // Modülü doğrula
    if (verifyModule()) {
         Reporter.reportError(SourceLocation(), "IR üretimi: Üretilen LLVM Modülü hatalı.");
         // Hata durumunda null pointer veya hata modülü döndürülebilir
         return nullptr; // Hata varsa modülü döndürme
    }

    return std::move(Module); // Tamamlanmış modülü döndür (sahipliği CompilerDriver'a geçer)
}

// Modülü doğrulama
bool IRGenerator::verifyModule() {
     // LLVM Modülünü doğrular. Hataları stderr'a yazar.
     // verifyModule fonksiyonu hata bulursa true döndürür.
    if (llvm::verifyModule(*Module, &llvm::errs())) { // &llvm::errs() doğrulama hatalarını stderr'a yazar
        Reporter.reportError(SourceLocation(), "LLVM Modül Doğrulaması Başarısız!");
        return true; // Doğrulama başarısız
    }
    return false; // Doğrulama başarılı
}


// Spesifik Bildirim IR Üretim Fonksiyonları

void IRGenerator::generateDecl(ast::DeclAST* decl) {
     if (!decl) return;
    // Bildirim türüne göre uygun IR üretim fonksiyonunu çağır
    if (auto funcDecl = dynamic_cast<ast::FunctionDeclAST*>(decl)) {
        generateFunctionDecl(funcDecl);
    }
    // TODO: generateStructDecl, generateEnumDecl, generateTraitDecl vb. (Eğer üst düzeyde runtime kod üretiyorlarsa)
    // Genellikle tip tanımları generateType içinde halledilir.

    else {
        Reporter.reportError(decl->loc, "IR üretimi: Bilinmeyen bildirim türü AST düğümü.");
    }
}

void IRGenerator::generateFunctionDecl(ast::FunctionDeclAST* funcDecl) {
    // Fonksiyon tipini LLVM tipine çevir
    llvm::Type* funcLLVMType = generateType(Types.getFunctionType(funcDecl->ReturnType.getResolvedType(), funcDecl->Params.getResolvedTypes())); // Varsayalım Type* artık AST düğümünde

    if (!funcLLVMType || funcLLVMType->isVoidTy()) { // Void tipi fonksiyon pointer'ları genellikle invalid kabul edilir
         Reporter.reportError(funcDecl->loc, "IR üretimi: Fonksiyon tipi geçerli değil.");
         return;
    }
    // FunctionType* nesnesinin kendisini alın
    auto llvmFuncType = dynamic_cast<llvm::FunctionType*>(funcLLVMType);
     if (!llvmFuncType) {
          Reporter.reportError(funcDecl->loc, "IR üretimi: Çevrilen LLVM tipi bir fonksiyon tipi değil.");
          return;
     }


    // LLVM Function nesnesini oluştur
    llvm::Function* llvmFunc = llvm::Function::Create(
        llvmFuncType,
        llvm::Function::ExternalLinkage, // Bağlantı türü (ExternalLinkage: C++'taki global fonksiyon gibi dışarıdan erişilebilir)
        funcDecl->Name,                  // Fonksiyon adı
        *Module                          // Ait olduğu modül
    );

    // Symbol tablosundaki fonksiyon sembolüne LLVM Function* pointer'ını kaydet
     Symbol* funcSymbol = Symbols.lookup(funcDecl->Name); // Sembolü SymbolTable'dan al (veya AST'de bağlı olsun)
     if (funcSymbol) funcSymbol->irValue = llvmFunc; // Symbol struct'ına irValue eklendiğini varsayalım
     else Reporter.reportError(funcDecl->loc, "IR üretimi: Fonksiyon sembolü bulunamadı."); // Dahili hata

    // Parametrelere isim ver (debugging ve erişim için)
    auto argIt = llvmFunc->arg_begin();
    for (const auto& param : funcDecl->Params) {
         param->resolvedSymbol->irValue = &(*argIt); // Parametre sembolüne LLVM Arg* pointer'ını kaydet
         argIt->setName(param->Name);
        ++argIt;
    }


    // Fonksiyon gövdesini oluştur (eğer varsa)
    if (funcDecl->Body) {
        // Giriş bloğunu oluştur ve builder'ın insert point'ini ayarla
        llvm::BasicBlock* entryBlock = createBasicBlock("entry", llvmFunc);
        setInsertPoint(entryBlock);

        // Parametreler için stack alanı ayır (Alloca) ve argüman değerlerini alana kaydet (Store)
        // Bu, parametrelere adres alarak referans geçirme veya mutasyon desteği sağlar.
        // eğer D++ parametreleri varsayılan olarak move/copy ile alıyorsa bu adım atlanabilir.
        argIt = llvmFunc->arg_begin();
         for (const auto& param : funcDecl->Params) {
             // Alloca instruction'ı giriş bloğuna yerleştir
             llvm::AllocaInst* alloca = createEntryBlockAlloca(llvmFunc, argIt->getType(), param->Name, param.getResolvedSymbol()); // Symbol* ast'den alınmalı
             // Argüman değerini Alloca'ya kaydet
             Builder->CreateStore(&(*argIt), alloca);
             // Parametre sembolünün irValue'sunu bu Alloca* olarak ayarla
              param.getResolvedSymbol()->irValue = alloca; // Symbol struct'ına irValue eklendiğini varsayalım
             ++argIt;
         }


        // Gövde içindeki deyimler için IR üret
        generateBlockStmt(funcDecl->Body.get());

        // Fonksiyon gövdesinin sonunda (eğer bir return deyimi yoksa ve geri dönüş tipi void ise)
        // implicit bir return void ekle.
        // Eğer son blokta terminatör yoksa (Branch veya Return), bu LLVM hatasıdır.
        // IRBuilder'ın insert point'i hala geçerliyse ve son blokta terminatör yoksa:
        if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
            if (funcDecl->ReturnType->isVoidType()) { // Geri dönüş tipi void
                Builder->CreateRetVoid();
            } else {
                 // Geri dönüş tipi void değil ama return yok. Semantic Analyzer'da hata olmalıydı.
                 // IR üretiminde hata veririz.
                 Reporter.reportError(funcDecl->Body->loc, "IR üretimi: Non-void fonksiyonun tüm yolları return içermiyor.");
                 // Geçici olarak undef değeriyle dönebilir veya hata bloğuna dallanabiliriz.
                  Builder->CreateRet(llvm::UndefValue::get(llvmFuncType->getReturnType()));
            }
        }
    }
      std::cerr << "DEBUG: Function '" << funcDecl->Name << "' IR generated." << std::endl; // Debug
}

// Spesifik Deyim IR Üretim Fonksiyonları

void IRGenerator::generateStmt(ast::StmtAST* stmt) {
     if (!stmt) return;
    // Deyim türüne göre uygun IR üretim fonksiyonunu çağır
    if (auto blockStmt = dynamic_cast<ast::BlockStmtAST*>(stmt)) {
        generateBlockStmt(blockStmt);
    } else if (auto ifStmt = dynamic_cast<ast::IfStmtAST*>(stmt)) {
        generateIfStmt(ifStmt);
    } else if (auto whileStmt = dynamic_cast<ast::WhileStmtAST*>(stmt)) {
        generateWhileStmt(whileStmt);
    } else if (auto forStmt = dynamic_cast<ast::ForStmtAST*>(stmt)) {
        generateForStmt(forStmt);
    } else if (auto returnStmt = dynamic_cast<ast::ReturnStmtAST*>(stmt)) {
        generateReturnStmt(returnStmt);
    } else if (auto breakStmt = dynamic_cast<ast::BreakStmtAST*>(stmt)) {
        generateBreakStmt(breakStmt);
    } else if (auto continueStmt = dynamic_cast<ast::ContinueStmtAST*>(stmt)) {
        generateContinueStmt(continueStmt);
    } else if (auto letStmt = dynamic_cast<ast::LetStmtAST*>(stmt)) {
        generateLetStmt(letStmt);
    } else if (auto exprStmt = dynamic_cast<ast::ExprStmtAST*>(stmt)) {
         // İfade deyimi: İfade için IR üret, sonucu at (side effect için)
         generateExpr(exprStmt->Expr.get());
    }
    // TODO: Diğer deyim türleri
    else {
        Reporter.reportError(stmt->loc, "IR üretimi: Bilinmeyen deyim türü AST düğümü.");
    }

    // Önemli Not: Bir deyim IR ürettikten sonra, Builder'ın insert point'i
    // doğru blokta kalmalı veya kontrol akışı değiştiyse yeni bloka ayarlanmalıdır.
    // generateIfStmt, generateWhileStmt gibi kontrol akışı deyimleri Builder'ın insert point'ini yönetir.
}

void IRGenerator::generateBlockStmt(ast::BlockStmtAST* blockStmt) {
     // Blok deyimi yeni bir LLVM BasicBlock oluşturmaz, mevcut bloğun içinde devam eder.
     // Ancak block scope'taki değişkenler için Alloca'lar genellikle fonksiyonun giriş bloğuna konur.
     // Semantic Analyzer kapsamları yönetir. IR Generator sadece içindeki deyimleri işler.
     // Blok sonunda Owned değişkenler için drop çağrıları gerekebilir (eğer Semantic Analyzer bunu belirlediyse).

    for (const auto& stmt : blockStmt->Statements) {
        generateStmt(stmt.get());
        // Eğer önceki deyim bir terminatör ürettiyse (return, break, continue),
        // Builder'ın insert point'i null olabilir. Sonraki deyimleri üretmeden kontrol et.
        if (!Builder->GetInsertBlock()) break;
    }
     // TODO: Kapsam sonu drop çağrıları (SymbolTable'da veya Sembolde drop bayrağı varsa)
}

void IRGenerator::generateIfStmt(ast::IfStmtAST* ifStmt) {
    // Koşul ifadesi için IR üret (bool tipinde olmalı, Sema kontrol eder)
    llvm::Value* condition = generateExpr(ifStmt->Condition.get());
    if (!condition) {
         // Hata oluştu, placeholder branch ekleyelim
         if (Builder->GetInsertBlock()) Builder->CreateUnreachable(); // Ulaşılamaz kod
         return;
    }

    // Koşul bool değilse (Sema'da hata olsa da), 1-bit integer'a cast etmeye çalışalım
    condition = Builder->CreateICmpNE(condition, llvm::ConstantInt::getFalse(*Context), "ifcond");


    // Then, Else (isteğe bağlı) ve Merge bloklarını oluştur
    llvm::Function* parentFunc = getCurrentFunction(); // Şu anki fonksiyonu al
    llvm::BasicBlock* thenBlock = createBasicBlock("then", parentFunc);
    llvm::BasicBlock* elseBlock = nullptr; // Opsiyonel
    if (ifStmt->ElseBlock) {
        elseBlock = createBasicBlock("else", parentFunc);
    }
    llvm::BasicBlock* mergeBlock = createBasicBlock("merge", parentFunc);


    // Koşul sonucuna göre dallanma (conditional branch)
    // Eğer else yoksa, false durumunda merge block'a dallanır.
    Builder->CreateCondBr(condition, thenBlock, elseBlock ? elseBlock : mergeBlock);


    // Then bloğu IR üretimi
    setInsertPoint(thenBlock);
    generateStmt(ifStmt->ThenBlock.get());
    // Then bloğu sonunda terminatör yoksa merge block'a dallan
    if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
        Builder->CreateBr(mergeBlock);
    }


    // Else bloğu IR üretimi (varsa)
    if (elseBlock) {
        setInsertPoint(elseBlock);
        generateStmt(ifStmt->ElseBlock.get());
        // Else bloğu sonunda terminatör yoksa merge block'a dallan
         if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
            Builder->CreateBr(mergeBlock);
        }
    }

    // Merge bloğunu insert point yap
    setInsertPoint(mergeBlock);
    // Not: Merge bloğunda PHI düğümleri olabilir eğer then ve else blokları değer üretiyorsa
    // (örn: ternary operatör a ? b : c veya match ifadesi)
    // If deyimi bir değer döndürmez, bu yüzden burada PHI düğümü gerekmez.
}

void IRGenerator::generateWhileStmt(ast::WhileStmtAST* whileStmt) {
     // Döngü bloklarını oluştur
     llvm::Function* parentFunc = getCurrentFunction();
     llvm::BasicBlock* conditionBlock = createBasicBlock("loop.cond", parentFunc);
     llvm::BasicBlock* bodyBlock = createBasicBlock("loop.body", parentFunc);
     llvm::BasicBlock* exitBlock = createBasicBlock("loop.exit", parentFunc);

     // Break/Continue hedeflerini kaydet (LoopExitBlocks, LoopContinueBlocks vektörlerini kullan)
      LoopExitBlocks.push_back(exitBlock);
      LoopContinueBlocks.push_back(conditionBlock);

     // Mevcut bloktan koşul bloğuna dallan
     if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
         Builder->CreateBr(conditionBlock);
     } else {
         // Önceki blok bir terminatör içeriyorsa (örn: if içinde return), koşul bloğu unreachable olabilir.
         // Builder'ın insert point'i null ise, koşul bloğuna manuel olarak başlangıç noktası ayarlayın.
          setInsertPoint(conditionBlock); // Unreachable durumunda bile bir başlangıç noktası gerekir
     }


     // Koşul bloğu IR üretimi
     setInsertPoint(conditionBlock);
     llvm::Value* condition = generateExpr(whileStmt->Condition.get());
     if (!condition) {
          // Hata oluştu, çıkış bloğuna dallanalım
          if (Builder->GetInsertBlock()) Builder->CreateUnreachable();
          setInsertPoint(exitBlock); // Exit bloğuna insert point'i taşı
     } else {
         // Koşul bool değilse cast et
         condition = Builder->CreateICmpNE(condition, llvm::ConstantInt::getFalse(*Context), "loopcond");
         // Koşul sonucuna göre dallan
         Builder->CreateCondBr(condition, bodyBlock, exitBlock);
     }


     // Gövde bloğu IR üretimi
     setInsertPoint(bodyBlock);
     generateStmt(whileStmt->Body.get());
     // Gövde sonunda terminatör yoksa koşul bloğuna geri dallan
     if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
         Builder->CreateBr(conditionBlock);
     }


     // Döngü hedeflerini temizle
      LoopExitBlocks.pop_back();
      LoopContinueBlocks.pop_back();

     // Çıkış bloğunu insert point yap
     setInsertPoint(exitBlock);
     // Not: Eğer hiçbir yerden exitBlock'a dallanmıyorsa (örn: sonsuz döngü veya her zaman return), bu blok ölü kod olabilir.
     // LLVM optimizasyonları bunu temizler.
}

void IRGenerator::generateForStmt(ast::ForStmtAST* forStmt) {
    // For döngüsü genellikle bir While döngüsüne çevrilebilir.
    // Init kısmı döngüden önce bir kez çalışır.
    // Condition kısmı her iterasyon başında kontrol edilir.
    // LoopEnd kısmı her iterasyon sonunda çalışır.
    // Body kısmı döngü gövdesidir.

    // Init deyimini üret (döngü dışında)
    if (forStmt->Init) {
        generateStmt(forStmt->Init.get());
        // Eğer Init bir terminatör ürettiyse (return), buradan çık.
        if (!Builder->GetInsertBlock()) return;
    }

    // Döngü bloklarını oluştur
    llvm::Function* parentFunc = getCurrentFunction();
    llvm::BasicBlock* conditionBlock = createBasicBlock("for.cond", parentFunc);
    llvm::BasicBlock* bodyBlock = createBasicBlock("for.body", parentFunc);
    llvm::BasicBlock* loopEndBlock = createBasicBlock("for.loopend", parentFunc); // LoopEnd ifadesinin çalışacağı blok
    llvm::BasicBlock* exitBlock = createBasicBlock("for.exit", parentFunc);

     // Break/Continue hedeflerini kaydet
      LoopExitBlocks.push_back(exitBlock);
      LoopContinueBlocks.push_back(conditionBlock);


    // Mevcut bloktan koşul bloğuna dallan
    if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
        Builder->CreateBr(conditionBlock);
    } else {
         setInsertPoint(conditionBlock);
    }

    // Koşul bloğu IR üretimi
    setInsertPoint(conditionBlock);
    llvm::Value* condition = nullptr;
    if (forStmt->Condition) {
        condition = generateExpr(forStmt->Condition.get());
        if (!condition) {
             // Hata oluştu, çıkışa dallan
             if (Builder->GetInsertBlock()) Builder->CreateUnreachable();
             setInsertPoint(exitBlock); return;
        }
        condition = Builder->CreateICmpNE(condition, llvm::ConstantInt::getFalse(*Context), "forcond");
    } else {
        // Koşul yoksa sonsuz döngü varsayılır (true)
        condition = llvm::ConstantInt::getTrue(*Context);
    }
    // Koşul sonucuna göre dallan
    Builder->CreateCondBr(condition, bodyBlock, exitBlock);


    // Gövde bloğu IR üretimi
    setInsertPoint(bodyBlock);
    generateStmt(forStmt->Body.get());
    // Gövde sonunda terminatör yoksa loopEnd bloğuna dallan
    if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
        Builder->CreateBr(loopEndBlock);
    }


    // LoopEnd bloğu IR üretimi
     setInsertPoint(loopEndBlock);
     if (forStmt->LoopEnd) {
         // LoopEnd ifadesini üret, sonucu at (side effect için)
         generateExpr(forStmt->LoopEnd.get());
          // Eğer LoopEnd bir terminatör ürettiyse (örn: return), bu blok zaten sonlanmıştır.
          if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
             // LoopEnd sonunda terminatör yoksa koşul bloğuna geri dallan
             Builder->CreateBr(conditionBlock);
          }
     } else {
          // LoopEnd yoksa doğrudan koşul bloğuna geri dallan
         if (Builder->GetInsertBlock() && !Builder->GetInsertBlock()->getTerminator()) {
            Builder->CreateBr(conditionBlock);
         }
     }


    // Döngü hedeflerini temizle
     LoopExitBlocks.pop_back();
     LoopContinueBlocks.pop_back();

    // Çıkış bloğunu insert point yap
    setInsertPoint(exitBlock);
}


void IRGenerator::generateReturnStmt(ast::ReturnStmtAST* returnStmt) {
    llvm::Value* returnValue = nullptr;
    if (returnStmt->ReturnValue) {
        // Dönüş değeri ifadesi için IR üret
        returnValue = generateExpr(returnStmt->ReturnValue.get());
        // TODO: Dönüş değeri tipinin fonksiyonun dönüş tipi ile uyumlu olduğunu ve gerekirse cast edilmesi gerektiğini kontrol et (Sema yaptı, burada sadece IR cast'i üretilir)
         Type* funcReturnType = getCurrentFunction()->getReturnType(); // LLVM Function'dan LLVM Return Tipini al
         Type* returnedLLVMType = returnValue->getType(); // Üretilen değerin LLVM tipi
         if (returnedLLVMType != funcReturnType) {
            returnValue = createTypeCast(returnValue, funcReturnType); // Cast üret
         }
    } else {
        // Dönüş değeri yoksa (void fonksiyonlar için) null pointer veya void değeri kullanılır
        // LLVM'de RetInst void veya Value* alır
    }

    // Ret instruction'ı oluştur
    if (returnValue) {
        Builder->CreateRet(returnValue);
    } else {
        Builder->CreateRetVoid();
    }

    // Önemli: Return bir terminatör instruction'dır. Bu bloktan sonra IR üretilmeyecektir.
    // Builder'ın insert point'ini null yapın veya yeni bir unreachable blok oluşturun.
    // Builder'ın insert point'ini null yapmak daha güvenlidir.
    Builder->ClearInsertionPoint();
}

void IRGenerator::generateBreakStmt(ast::BreakStmtAST* breakStmt) {
    // Break deyimi, içinde bulunduğu en yakın döngünün çıkış bloğuna dallanır.
    // LoopExitBlocks vektörünün sonundaki BasicBlock'a dallan.
     if (!LoopExitBlocks.empty()) {
         Builder->CreateBr(LoopExitBlocks.back());
         Builder->ClearInsertionPoint(); // Terminasyon sonrası insert point'i temizle
     } else {
         Reporter.reportError(breakStmt->loc, "IR üretimi: Break deyimi döngü içinde değil."); // Sema hatası olmalıydı
         if (Builder->GetInsertBlock()) Builder->CreateUnreachable(); // Hata durumunda unreachable
         Builder->ClearInsertionPoint();
     }
    Reporter.reportNote(breakStmt->loc, "IR üretimi: Break deyimi implement edilmedi (Loop hedef takibi gerekli)."); // Geçici mesaj
    if (Builder->GetInsertBlock()) Builder->CreateUnreachable();
    Builder->ClearInsertionPoint();
}

void IRGenerator::generateContinueStmt(ast::ContinueStmtAST* continueStmt) {
     // Continue deyimi, içinde bulunduğu en yakın döngünün bir sonraki iterasyon başlangıç bloğuna dallanır.
     // LoopContinueBlocks vektörünün sonundaki BasicBlock'a dallan.
      if (!LoopContinueBlocks.empty()) {
          Builder->CreateBr(LoopContinueBlocks.back());
          Builder->ClearInsertionPoint(); // Terminasyon sonrası insert point'i temizle
      } else {
          Reporter.reportError(continueStmt->loc, "IR üretimi: Continue deyimi döngü içinde değil."); // Sema hatası olmalıydı
          if (Builder->GetInsertBlock()) Builder->CreateUnreachable(); // Hata durumunda unreachable
          Builder->ClearInsertionPoint();
      }
     Reporter.reportNote(continueStmt->loc, "IR üretimi: Continue deyimi implement edilmedi (Loop hedef takibi gerekli)."); // Geçici mesaj
     if (Builder->GetInsertBlock()) Builder->CreateUnreachable();
     Builder->ClearInsertionPoint();
}


void IRGenerator::generateLetStmt(ast::LetStmtAST* letStmt) {
     // Let deyimi bir değişken bildirir. Bu değişken için stack'te yer ayrılır (Alloca).
     // Semantic Analyzer'da LetStmtAST'ye bağlı olan Symbol'ün irValue alanı bu Alloca* olarak ayarlanacaktır.

     // Pattern analizi ve değişken bağlama Sema'da yapıldı. Pattern genellikle IdentifierPattern olacaktır.
      if (auto idPattern = dynamic_cast<ast::IdentifierPatternAST*>(letStmt->Pattern.get())) {
          Symbol* varSymbol = idPattern->resolvedSymbol; // Sema tarafından bağlandı
          if (varSymbol) {
              llvm::Type* varLLVMType = generateType(varSymbol->type); // Değişkenin LLVM tipini al
              if (!varLLVMType) { ... hata ... }
     //
     //         // Fonksiyonun giriş bloğuna Alloca instruction'ı yerleştir
              llvm::Function* parentFunc = getCurrentFunction();
              llvm::AllocaInst* alloca = createEntryBlockAlloca(parentFunc, varLLVMType, varSymbol->Name, varSymbol);
     //
     //         // Symbol'ün irValue'sunu Alloca* olarak kaydet
               varSymbol->irValue = alloca; // Zaten createEntryBlockAlloca içinde yapılıyor
     //
     //         // Başlangıç değeri varsa, değeri hesapla ve Alloca'ya kaydet (Store)
              if (letStmt->Initializer) {
                  llvm::Value* initializerValue = generateExpr(letStmt->Initializer.get());
                  if (initializerValue) {
     //                 // Initializer değeri tipinin Alloca'nın tipi ile uyumlu olduğunu ve gerekirse cast edilmesi gerektiğini kontrol et (Sema yaptı, burada IR cast)
                       initializerValue = createTypeCast(initializerValue, varLLVMType);
                      Builder->CreateStore(initializerValue, alloca);
                  }
              } else {
     //              // Başlangıç değeri yoksa, değişken Uninitialized durumdadır.
                   // Sema bunu kontrol eder. IR'da sadece yer ayrılır.
              }
          } else {
              Reporter.reportError(letStmt->loc, "IR üretimi: Let deyimi sembolü bulunamadı."); // Dahili hata
          }
      } else {
     //      // Daha karmaşık patternler (struct deconstruction, tuple pattern vb.) için IR üretimi
     //      // Bu, pattern'a göre değerleri parçalayıp ayrı Alloca'lara kaydetmeyi içerir.
           Reporter.reportError(letStmt->Pattern->loc, "IR üretimi: Karmaşık patternler için IR üretimi henüz implement edilmedi."); // Geçici mesaj
     //      // Hata sonrası toparlanma
      }
     Reporter.reportNote(letStmt->loc, "IR üretimi: Let deyimi implement edilmedi (Pattern ve Symbol bağlama gerekli)."); // Geçici mesaj
}

// Spesifik İfade IR Üretim Fonksiyonları (Hepsi llvm::Value* döndürür)

llvm::Value* IRGenerator::generateExpr(ast::ExprAST* expr) {
     if (!expr) {
          // Reporter.reportError(SourceLocation(), "IR üretimi: Null ifade düğümü.");
         // Hata durumunda güvenli bir undef değeri veya null pointer döndür
         return llvm::UndefValue::get(llvm::Type::getVoidTy(*Context)); // Veya uygun bir varsayılan
     }

    // İfade türüne göre uygun IR üretim fonksiyonunu çağır
    if (auto intExpr = dynamic_cast<ast::IntegerExprAST*>(expr)) {
        return generateIntegerExpr(intExpr);
    } else if (auto floatExpr = dynamic_cast<ast::FloatingExprAST*>(expr)) {
        return generateFloatingExpr(floatExpr);
    } else if (auto stringExpr = dynamic_cast<ast::StringExprAST*>(expr)) {
        return generateStringExpr(stringExpr);
    } else if (auto boolExpr = dynamic_cast<ast::BooleanExprAST*>(expr)) {
        return generateBooleanExpr(boolExpr);
    } else if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(expr)) {
        return generateIdentifierExpr(idExpr);
    } else if (auto binOpExpr = dynamic_cast<ast::BinaryOpExprAST*>(expr)) {
        return generateBinaryOpExpr(binOpExpr);
    } else if (auto unOpExpr = dynamic_cast<ast::UnaryOpExprAST*>(expr)) {
         return generateUnaryOpExpr(unOpExpr);
    } else if (auto callExpr = dynamic_cast<ast::CallExprAST*>(expr)) {
         return generateCallExpr(callExpr);
    } else if (auto memberExpr = dynamic_cast<ast::MemberAccessExprAST*>(expr)) {
         return generateMemberAccessExpr(memberExpr);
    } else if (auto arrayExpr = dynamic_cast<ast::ArrayAccessExprAST*>(expr)) {
         return generateArrayAccessExpr(arrayExpr);
    } else if (auto newExpr = dynamic_cast<ast::NewExprAST*>(expr)) {
         return generateNewExpr(newExpr);
    } else if (auto matchExpr = dynamic_cast<ast::MatchExprAST*>(expr)) {
         return generateMatchExpr(matchExpr);
    } else if (auto borrowExpr = dynamic_cast<ast::BorrowExprAST*>(expr)) {
         return generateBorrowExpr(borrowExpr);
    } else if (auto derefExpr = dynamic_cast<ast::DereferenceExprAST*>(expr)) {
         return generateDereferenceExpr(derefExpr);
    }
    // TODO: Diğer ifade türleri
    else {
        Reporter.reportError(expr->loc, "IR üretimi: Bilinmeyen ifade türü AST düğümü.");
        return llvm::UndefValue::get(llvm::Type::getVoidTy(*Context)); // Hata durumunda
    }
}


llvm::Value* IRGenerator::generateIntegerExpr(ast::IntegerExprAST* expr) {
    // Tamsayı literalini LLVM ConstantInt'e çevir
    // Tipin boyutunu Semantik Analizden veya TypeSystem'dan alın.
     sema::Type* dppType = expr->resolvedType; // Varsayalım AST düğümüne tip eklendi
     llvm::Type* llvmType = generateType(dppType); // LLVM tipini al

    // Şimdilik varsayılan int32 tipi varsayalım
    llvm::Type* llvmType = llvm::Type::getInt32Ty(*Context);
    return llvm::ConstantInt::get(llvmType, expr->Value, true); // true: işaretli (signed)
}

llvm::Value* IRGenerator::generateFloatingExpr(ast::FloatingExprAST* expr) {
    // Kayan noktalı literalini LLVM ConstantFP'ye çevir
     sema::Type* dppType = expr->resolvedType;
     llvm::Type* llvmType = generateType(dppType);

    // Şimdilik varsayılan float32 tipi varsayalım
    llvm::Type* llvmType = llvm::Type::getFloatTy(*Context); // veya DoubleTy

    return llvm::ConstantFP::get(llvmType, expr->Value);
}

llvm::Value* IRGenerator::generateStringExpr(ast::StringExprAST* expr) {
    // String literalini LLVM global constant string'e çevir
    // Bu, LLVM IR'da @.str = private unnamed_addr constant [N x i8] c"...", align 1 gibi görünür.
    // Builder->CreateGlobalStringPtr, bu global sabitin pointer'ını döndürür (i8* tipi).
    return Builder->CreateGlobalStringPtr(expr->Value);
}

llvm::Value* IRGenerator::generateBooleanExpr(ast::BooleanExprAST* expr) {
    // Boolean literalini LLVM ConstantInt'e çevir (i1 tipi)
    return llvm::ConstantInt::getBool(*Context, expr->Value);
}


llvm::Value* IRGenerator::generateIdentifierExpr(ast::IdentifierExprAST* expr) {
    // Sembol tablosundan identifier'ın sembolünü al (Sema tarafından çözüldü)
     sema::Symbol* symbol = expr->resolvedSymbol; // Varsayalım AST düğümüne sembol eklendi

    // Symbol'ün LLVM Value* bilgisini al (LetStmt sırasında Alloca* olarak kaydedildi)
     if (symbol && symbol->irValue) {
    //     // Eğer sembol bir değişken (Variable, Parameter) ise, bu bir AllocaInst*'dır.
    //     // Değişkenin *değerini* elde etmek için Load instruction'ı üret.
         if (symbol->Kind == sema::SymbolKind::Variable || symbol->Kind == sema::SymbolKind::Parameter) {
    //          // TODO: Sahiplik/Borrowing: Eğer sembol Moved veya Dropped ise burada hata kontrolü (Sema yaptı)
    //          // Eğer sembol BorrowedMutable ise, direct Load yapmamalı, sadece mutable referans üzerinden erişilmeli.
               checkSymbolUsability(symbol, expr->loc, "kullanım"); // BorrowChecker'dan çağrılabilir
    //
              return Builder->CreateLoad(generateType(symbol->type), symbol->irValue, symbol->Name + "_val"); // Load instruction
    //
         } else if (symbol->Kind == sema::SymbolKind::Function) {
    //         // Eğer sembol bir fonksiyon ise, LLVM Function* pointer'ını döndür.
             return symbol->irValue; // Bu zaten bir LLVM Function*
         }
    //      // TODO: Diğer sembol türleri (Type, Struct, Enum vb.) identifier olarak kullanılırsa nasıl IR üretilir?
    //      // Tip isimleri genellikle runtime değeri temsil etmez, compile-time bilgi sağlar.
         Reporter.reportError(expr->loc, "IR üretimi: Identifier bilinmeyen sembol türüne sahip.");
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata durumunda
    //
     } else {
          Reporter.reportError(expr->loc, "IR üretimi: Identifier sembolü veya LLVM değeri bulunamadı."); // Dahili hata (Sema çözememiş veya IRGenerator kaydetmemiş)
          return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata durumunda
     }
    Reporter.reportNote(expr->loc, "IR üretimi: Identifier ifadesi implement edilmedi (Symbol bağlama gerekli)."); // Geçici mesaj
     return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata veya placeholder
}

llvm::Value* IRGenerator::generateBinaryOpExpr(ast::BinaryOpExprAST* expr) {
    // Sol ve sağ operandlar için IR üret
    llvm::Value* left = generateExpr(expr->Left.get());
    llvm::Value* right = generateExpr(expr->Right.get());

    if (!left || !right) {
        // Hata oluştu
         Reporter.reportError(expr->loc, "IR üretimi: İkili operatör operandları için IR üretilemedi.");
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata durumunda
    }

    // Operand tiplerini al (Sema tarafından çözüldü)
     sema::Type* leftDppType = expr->Left->resolvedType;
     sema::Type* rightDppType = expr->Right->resolvedType;

    // LLVM operand tiplerini al
    llvm::Type* leftLLVMType = left->getType();
    llvm::Type* rightLLVMType = right->getType();

    // TODO: Sema, operatörün tipler için geçerli olduğunu kontrol etti.
    // Burada sadece uygun LLVM instruction'ı üretilir.
    // Tipler farklıysa ve uyumluysa (örn: int + float), LLVM'de implicit cast gerekebilir.
    // createTypeCast fonksiyonunu kullanın. Sema hangi cast'in geçerli olduğunu belirler.
    // Genellikle, daha dar tipten daha geniş tipe cast yapılır.
    // Örn: int + float -> (float)int + float
    // double + int -> double + (double)int

     if (leftLLVMType != rightLLVMType) {
    //     // Auto-promosyon veya implicit cast kuralları (Sema tarafından belirlenen)
    //     // Basit sayısal promosyon örneği:
         if (leftLLVMType->isFloatingPointTy() && rightLLVMType->isIntegerTy()) {
              right = Builder->CreateSIToFP(right, leftLLVMType, "cast"); // Signed Int To Floating Point
         } else if (leftLLVMType->isIntegerTy() && rightLLVMType->isFloatingPointTy()) {
              left = Builder->CreateSIToFP(left, rightLLVMType, "cast");
         }
    //     // TODO: Diğer cast durumları (pointer-integer, integer-integer farklı boyutlarda vb.)
          // createTypeCast fonksiyonunu kullanmak daha geneldir.
     }


    // Operatör türüne göre uygun LLVM instruction'ı oluştur
    switch (expr->Op) {
        case lexer::TokenType::Op_Plus: // +
            if (leftLLVMType->isIntegerTy()) return Builder->CreateAdd(left, right, "addtmp");
            if (leftLLVMType->isFloatingPointTy()) return Builder->CreateFAdd(left, right, "faddtmp");
            // TODO: Pointer aritmetiği, Struct/Array toplama (eğer dil destekliyorsa)
            Reporter.reportError(expr->loc, "IR üretimi: '+' operatörü için desteklenmeyen tip."); return llvm::UndefValue::get(generateType(expr->resolvedType));
        case lexer::TokenType::Op_Minus: // -
            if (leftLLVMType->isIntegerTy()) return Builder->CreateSub(left, right, "subtmp");
            if (leftLLVMType->isFloatingPointTy()) return Builder->CreateFSub(left, right, "fsubtmp");
             Reporter.reportError(expr->loc, "IR üretimi: '-' operatörü için desteklenmeyen tip."); return llvm::UndefValue::get(generateType(expr->resolvedType));
        case lexer::TokenType::Op_Star: // *
            if (leftLLVMType->isIntegerTy()) return Builder->CreateMul(left, right, "multmp");
            if (leftLLVMType->isFloatingPointTy()) return Builder->CreateFMul(left, right, "fmultmp");
             Reporter.reportError(expr->loc, "IR üretimi: '*' operatörü için desteklenmeyen tip."); return llvm::UndefValue::get(generateType(expr->resolvedType));
        case lexer::TokenType::Op_Slash: // /
            if (leftLLVMType->isIntegerTy()) { // Tamsayı bölme (işaretli/işaretsiz)
                  sema::Type* leftDppType = expr->Left->resolvedType;
                  if (leftDppType->isSignedInteger()) return Builder->CreateSDiv(left, right, "sdivtmp");
                  else return Builder->CreateUDiv(left, right, "udivtmp"); // İşaretsiz bölme
                 return Builder->CreateSDiv(left, right, "sdivtmp"); // Varsayılan olarak işaretli
            }
            if (leftLLVMType->isFloatingPointTy()) return Builder->CreateFDiv(left, right, "fdivtmp");
             Reporter.reportError(expr->loc, "IR üretimi: '/' operatörü için desteklenmeyen tip."); return llvm::UndefValue::get(generateType(expr->resolvedType));
        case lexer::TokenType::Op_Percent: // %
             if (leftLLVMType->isIntegerTy()) { // Tamsayı modül (işaretli/işaretsiz)
                  sema::Type* leftDppType = expr->Left->resolvedType;
                  if (leftDppType->isSignedInteger()) return Builder->CreateSRem(left, right, "sremtmp"); // Signed Remainder
                  else return Builder->CreateURem(left, right, "uremtmp"); // Unsigned Remainder
                 return Builder->CreateSRem(left, right, "sremtmp"); // Varsayılan işaretli
            }
             Reporter.reportError(expr->loc, "IR üretimi: '%' operatörü için desteklenmeyen tip."); return llvm::UndefValue::get(generateType(expr->resolvedType));

        case lexer::TokenType::Op_EqualEqual: // ==
        case lexer::TokenType::Op_NotEqual:   // !=
        case lexer::TokenType::Op_LessThan:   // <
        case lexer::TokenType::Op_GreaterThan:// >
        case lexer::TokenType::Op_LessEqual:  // <=
        case lexer::TokenType::Op_GreaterEqual:// >=
            // Karşılaştırma operatörleri. Sonuç i1 (bool).
            if (leftLLVMType->isIntegerTy()) { // Tamsayı karşılaştırma
                llvm::CmpInst::Predicate predicate;
                switch (expr->Op) {
                     case lexer::TokenType::Op_EqualEqual: predicate = llvm::CmpInst::ICMP_EQ; break;
                     case lexer::TokenType::Op_NotEqual:   predicate = llvm::CmpInst::ICMP_NE; break;
                     case lexer::TokenType::Op_LessThan:   predicate = llvm::CmpInst::ICMP_SLT; break; // SLT: Signed Less Than
                     case lexer::TokenType::Op_GreaterThan:predicate = llvm::CmpInst::ICMP_SGT; break; // SGT: Signed Greater Than
                     case lexer::TokenType::Op_LessEqual:  predicate = llvm::CmpInst::ICMP_SLE; break; // SLE: Signed Less or Equal
                     case lexer::TokenType::Op_GreaterEqual:predicate = llvm::CmpInst::ICMP_SGE; break; // SGE: Signed Greater or Equal
                     default: predicate = llvm::CmpInst::BAD_ICMP_PREDICATE; // Olmamalı
                }
                 // TODO: İşaretsiz tamsayılar için UGT, ULT, UGE, ULE kullan
                 return Builder->CreateICmp(predicate, left, right, "icmp");
            }
             if (leftLLVMType->isFloatingPointTy()) { // Kayan noktalı karşılaştırma
                 llvm::CmpInst::Predicate predicate;
                 switch (expr->Op) {
                      case lexer::TokenType::Op_EqualEqual: predicate = llvm::CmpInst::FCMP_OEQ; break; // OEQ: Ordered Equal
                      case lexer::TokenType::Op_NotEqual:   predicate = llvm::CmpInst::FCMP_ONE; break; // ONE: Ordered Not Equal
                      case lexer::TokenType::Op_LessThan:   predicate = llvm::CmpInst::FCMP_OLT; break; // OLT: Ordered Less Than
                      case lexer::TokenType::Op_GreaterThan:predicate = llvm::CmpInst::FCMP_OGT; break; // OGT: Ordered Greater Than
                      case lexer::TokenType::Op_LessEqual:  predicate = llvm::CmpInst::FCMP_OLE; break; // OLE: Ordered Less or Equal
                      case lexer::TokenType::Op_GreaterEqual:predicate = llvm::CmpInst::FCMP_OGE; break; // OGE: Ordered Greater or Equal
                      default: predicate = llvm::CmpInst::BAD_FCMP_PREDICATE; // Olmamalı
                 }
                  // "O" predicate'leri NaN durumunu ele alır, "U" (Unordered) NaN'ı farklı ele alır. Genellikle Ordered kullanılır.
                 return Builder->CreateFCmp(predicate, left, right, "fcmp");
             }
             // TODO: Pointer karşılaştırma (EQ, NE)
              Reporter.reportError(expr->loc, "IR üretimi: Karşılaştırma operatörü için desteklenmeyen tip."); return llvm::UndefValue::get(generateType(expr->resolvedType));

        case lexer::TokenType::Op_And: // && (Mantıksal AND)
             // LLVM'de mantıksal AND genellikle i1 tipleri üzerinde AND instruction'ıdır.
             // Ancak kısa devre (short-circuiting) davranışı dallanma (branch) ile sağlanır.
             // Bu, generateExpr içinde doğrudan üretmek yerine, IfStmt gibi kontrol akışı yapılarında kullanılır.
             // Eğer && bir ifade olarak geçiyorsa (örn: a = b && c;), kısa devre olmadan i1 AND üretilebilir.
             // Sema &&'in bool operandları olmasını garanti eder.
             if (leftLLVMType->isIntegerTy(1) && rightLLVMType->isIntegerTy(1)) {
                 return Builder->CreateLogicalAnd(left, right, "andtmp"); // Veya CreateAnd
             }
             Reporter.reportError(expr->loc, "IR üretimi: Mantıksal AND operatörü için bool tipi bekleniyor."); return llvm::UndefValue::get(generateType(expr->resolvedType));
        case lexer::TokenType::Op_Or: // || (Mantıksal OR)
             // Kısa devre veya i1 OR.
             if (leftLLVMType->isIntegerTy(1) && rightLLVMType->isIntegerTy(1)) {
                  return Builder->CreateLogicalOr(left, right, "ortmp"); // Veya CreateOr
             }
              Reporter.reportError(expr->loc, "IR üretimi: Mantıksal OR operatörü için bool tipi bekleniyor."); return llvm::UndefValue::get(generateType(expr->resolvedType));

         // TODO: Diğer operatörler (Bitwise, Shift, Compound Assignment += vb., ., ->, ::)
         // Bileşik atamalar (+=) bir okuma, işlem ve kaydetme sırasıdır. generateAssignment içinde işlenebilir.
         // Üye erişimi (., ->) GEP instruction'ı kullanır. generateMemberAccessExpr içinde işlenebilir.
         // :: genellikle kapsam çözümlemedir, IR'da doğrudan karşılığı yoktur, ismin çözülmüş haline dönüşür.

        default:
            Reporter.reportError(expr->loc, "IR üretimi: Bilinmeyen veya desteklenmeyen ikili operatör AST düğümü.");
            return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata durumunda
    }
}

llvm::Value* IRGenerator::generateUnaryOpExpr(ast::UnaryOpExprAST* expr) {
     llvm::Value* operand = generateExpr(expr->Operand.get());
    if (!operand) {
         Reporter.reportError(expr->loc, "IR üretimi: Tekli operatör operandı için IR üretilemedi.");
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
    }

    llvm::Type* operandLLVMType = operand->getType();

    switch (expr->Op) {
        case lexer::TokenType::Op_Minus: // Tekli eksi (-)
            if (operandLLVMType->isIntegerTy()) return Builder->CreateNeg(operand, "negtmp");
            if (operandLLVMType->isFloatingPointTy()) return Builder->CreateFNeg(operand, "fnegtmp");
             Reporter.reportError(expr->loc, "IR üretimi: Tekli '-' operatörü için desteklenmeyen tip."); return llvm::UndefValue::get(generateType(expr->resolvedType));
        case lexer::TokenType::Op_Not: // Mantıksal NOT (!)
             if (operandLLVMType->isIntegerTy(1)) return Builder->CreateNot(operand, "nottmp"); // i1 üzerinde bitwise NOT
             Reporter.reportError(expr->loc, "IR üretimi: Tekli '!' operatörü için bool tipi bekleniyor."); return llvm::UndefValue::get(generateType(expr->resolvedType));
        // TODO: Bitwise NOT (~)

        // Dereference (*) ve Borrow (&, &mut) ayrı AST düğümlerine taşındıysa bu case'ler burada olmamalıdır.
         case lexer::TokenType::Op_Star: // Dereference (*)
         case lexer::TokenType::Op_BitAnd: // Borrow (&)

        default:
            Reporter.reportError(expr->loc, "IR üretimi: Bilinmeyen veya desteklenmeyen tekli operatör AST düğümü.");
            return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
    }
}

llvm::Value* IRGenerator::generateCallExpr(ast::CallExprAST* expr) {
     // Çağrılan fonksiyonu (callee) bul
     // Sema, CalleeName'i çözdü ve AST düğümüne ilgili Symbol*'ü eklediğini varsayalım.
      sema::Symbol* calleeSymbol = expr->resolvedFunction; // AST düğümünde risolvedSymbol veya resolvedFunction
      if (!calleeSymbol || !calleeSymbol->irValue || calleeSymbol->Kind != sema::SymbolKind::Function) {
          Reporter.reportError(expr->loc, "IR üretimi: Fonksiyon çağrısı için çözümlenmiş fonksiyon bulunamadı."); // Sema hatası veya dahili hata
          return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
      }
      llvm::Function* calleeFunc = static_cast<llvm::Function*>(calleeSymbol->irValue);

    // Şimdilik doğrudan fonksiyon adı üzerinden LLVM Function'ı arayalım (Basitlik için)
     llvm::Function* calleeFunc = Module->getFunction(expr->CalleeName);
     if (!calleeFunc) {
         Reporter.reportError(expr->loc, "IR üretimi: LLVM Modülünde '" + expr->CalleeName + "' adında fonksiyon bulunamadı."); // Bağlantı hatası veya Sema hatası
          return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
     }


     // Argümanlar için IR üret
    std::vector<llvm::Value*> argValues;
    for (const auto& arg : expr->Args) {
        if (llvm::Value* argValue = generateExpr(arg.get())) {
             // TODO: Argüman tipinin parametre tipi ile uyumlu olduğunu ve gerekirse cast edilmesi gerektiğini kontrol et (Sema yaptı, burada IR cast)
              llvm::Type* paramLLVMType = calleeFunc->getFunctionType()->getParamType(argValues.size()); // Bu doğru olmayabilir, parametre sırası önemli
              argValue = createTypeCast(argValue, paramLLVMType);
             argValues.push_back(argValue);
        } else {
            Reporter.reportError(arg->loc, "IR üretimi: Fonksiyon çağrısı argümanı için IR üretilemedi.");
             // Hata argümanı için placeholder ekleyebiliriz veya tüm çağrıyı geçersiz sayabiliriz.
             // Şimdilik hata durumunda undef ekleyip devam edelim, LLVM doğrulaması bunu yakalayabilir.
             argValues.push_back(llvm::UndefValue::get(calleeFunc->getFunctionType()->getParamType(argValues.size())));
        }
    }

    // Argüman sayısı eşleşiyor mu kontrol et (Sema yaptı)
     if (argValues.size() != calleeFunc->arg_size()) {
         Reporter.reportError(expr->loc, "IR üretimi: Fonksiyon çağrısı argüman sayısı uyuşmuyor (LLVM seviyesi)."); // Dahili hata veya Sema hatası
         // IR üretimini durdurma veya hata değeri döndürme
         return llvm::UndefValue::get(generateType(expr->resolvedType));
     }


    // Call instruction'ı oluştur
    // Fonksiyonun geri dönüş tipi void değilse bir isim verin.
    if (calleeFunc->getReturnType()->isVoidTy()) {
        Builder->CreateCall(calleeFunc, argValues);
        return nullptr; // Void fonksiyonlar değer döndürmez, IR'da null pointer ile temsil edilir
    } else {
        return Builder->CreateCall(calleeFunc, argValues, "calltmp");
    }
}

llvm::Value* IRGenerator::generateMemberAccessExpr(ast::MemberAccessExprAST* expr) {
    // Taban ifadesi için IR üret (yapı, pointer, referans vb.)
    llvm::Value* base = generateExpr(expr->Base.get());
    if (!base) {
         Reporter.reportError(expr->loc, "IR üretimi: Üye erişimi taban ifadesi için IR üretilemedi.");
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
    }

    // Taban ifadesinin LLVM tipini al
    llvm::Type* baseLLVMType = base->getType();

    // Taban tipi pointer veya referans ise, işaret ettiği tipe ulaşmak için Load instruction'ı gerekebilir.
    // Veya üye erişimi doğrudan pointer/referans üzerinden GEP ile yapılır (Rust'ta böyle).
    // Eğer AST düğümü `isPointerAccess` (->) veya `isReferenceAccess` (implicit .) bilgisi içeriyorsa bunu kullanın.
     sema::Type* baseDppType = expr->Base->resolvedType; // Sema tarafından çözülen D++ tipi

    llvm::Value* basePtr = base; // Varsayılan olarak tabanın kendisi pointer/referans
    llvm::Type* baseStructOrUnionType = nullptr;

    // Eğer taban bir değer ise (Pointer veya Reference değil), üyelerine erişmek için önce adresini almak gerekir (eğer lvalue ise).
    // Bu karmaşıktır, genellikle üye erişiminin tabanı zaten bir pointer/referans (Alloca*, GEP sonucu, argüman referansı) olur.
    // Sema, üye erişiminin geçerli bir base'e yapıldığını kontrol etti.
    // Örneğin, bir StructType'ın değeri üzerinde '.' operatörü kullanılıyorsa, StructType* değerin *adresi* alınmalı.
     if (!baseExpr->isLValue) { ... hata veya geçici oluşturup kopyala ... }


    // Eğer base bir pointer veya referans ise, GEP direkt olarak pointer üzerinde çalışır.
    // Eğer base bir değer ise (struct value), GEP'den önce adresi (Alloca* gibi) alınmalıdır.
    // LLVM GEP instruction'ı bir pointer alır ve bir dizi indeks vererek yeni bir pointer üretir.
    // Struct üyelerine erişmek için ikinci indeks daima 0'dır, üçüncü indeks alanın indexidir.
    // Pointerlar ve Arrayler için ikinci indeks alanın offsetidir (genellikle 0), üçüncü indeks array/pointer indexidir.


    // Taban ifadenin tipine göre gerçek üye erişimi yapılacak tipi ve pointer durumunu belirle.
     if (baseLLVMType->isPointerTy()) {
         // Taban zaten bir pointer
         basePtr = base;
         baseStructOrUnionType = baseLLVMType->getPointerElementType(); // İşaret edilen tipi al
     } else {
         // Taban bir değer (Struct, Array, Tuple değeri olabilir)
         // Üye erişimi için değerin adresini almalıyız. Bu ancak base bir lvalue ise mümkündür.
          if (!expr->Base->isLValue) { ... hata (Sema yakaladı) ... }
         // Taban ifadenin adresini veren Value* SemanticAnalyzer tarafından AST'ye eklenmiş olabilir.
          basePtr = expr->Base->addressValue; // Örnek AST anotasyonu

         // Veya base bir fonksiyondan döndüyse/literal ise, geçici bir Alloca oluşturup değeri kaydedip o Alloca'nın adresini alabiliriz.
         // Bu da karmaşık. Varsayalım baseExpr sonucu her zaman bir pointer/referans veya lvalue'nun kendisidir.
         Reporter.reportError(expr->loc, "IR üretimi: Üye erişimi tabanı pointer/referans değil (Karmaşık LValue'lar implement edilmedi).");
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
     }


    // Üye adını bul (Sema tarafından çözüldü) ve StructType içinde indexini al
     sema::StructType* structType = dynamic_cast<sema::StructType*>(expr->Base->resolvedType->getBaseTypeIfPointerOrReference()); // Sema'dan D++ Struct Type'ı al
     if (!structType) { ... hata ... }
     const sema::StructField* field = structType->getField(expr->MemberName);
     int fieldIndex = structType->getFieldIndex(expr->MemberName); // TypeSystem veya StructType'da index bulma metodu olmalı
     if (fieldIndex < 0) { ... hata ... }

     // Şimdilik alan indexini hardcode edelim veya sema'dan geldiğini varsayalım.
     int fieldIndex = 0; // TODO: Gerçek alan indexini al (Sema tarafından çözülen StructField* üzerinden veya TypeSystem'dan)
     Reporter.reportNote(expr->loc, "IR üretimi: Üye erişimi alan indexi varsayılan (0)."); // Geçici mesaj


    // GetElementPtr (GEP) instruction'ı oluştur
    // GEP instruction'ı, bir dizinin, yapının veya vektörün bir elemanının *adresini* hesaplar.
    // İki formu vardır: Dizi/Pointer GEP ve Yapı (Struct) GEP.
    // Yapı GEP formatı: GEP <StructType>, <StructPointer>, i32 0, i32 <FieldIndex>
    // Pointer/Array GEP formatı: GEP <BaseType>, <Pointer>, i32 <Index1>, i32 <Index2>, ...
    // Üye erişimi genellikle Struct GEP'dir.
    llvm::Value* memberPtr = nullptr;
    if (baseStructOrUnionType && baseStructOrUnionType->isStructTy()) {
         memberPtr = Builder->CreateStructGEP(baseStructOrUnionType, basePtr, fieldIndex, expr->MemberName + "_ptr");
    }
    // TODO: Union üyelerine erişim
    // TODO: D'deki class member access (virtual fonksiyonlar, inheritence)
    else {
         Reporter.reportError(expr->loc, "IR üretimi: Üye erişimi beklenmeyen taban tipi.");
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
    }


    // Üye erişimi ifadesinin kendisi, genellikle alanın *değeridir* (Load instruction'ı gerekir).
    // Ancak atama sol tarafında (lvalue) ise Load gerekmez, GEP sonucu (pointer) kullanılır.
    // Sema tarafından belirlenen ifadenin lvalue/rvalue durumunu kullanın.
     if (expr->isLValue) { // Atama sol tarafı gibi kullanılıyor
         return memberPtr; // Pointer'ı döndür
     } else { // Normal kullanım (değer)
    //     // Alanın tipini LLVM tipine çevir
          sema::Type* memberDppType = field->type; // Sema'dan alanın D++ tipini al
          llvm::Type* memberLLVMType = generateType(memberDppType);
    //
    //      // TODO: Eğer alan mutable değilse ve konteyner borrowed shared ise, bu bir Shared Borrow kullanımıdır.
    //      // Eğer alan mutable ise ve konteyner borrowed mutable ise, bu bir Mutable Borrow kullanımıdır.
    //      // checkBorrowRules / checkSymbolUsability (Sema yaptı ama burada IR anlamında kontrol önemli)
    //
         llvm::Type* memberLLVMType = memberPtr->getType()->getPointerElementType(); // İşaret edilen tipi al
         return Builder->CreateLoad(memberLLVMType, memberPtr, expr->MemberName + "_val"); // Load instruction
     }

     Reporter.reportNote(expr->loc, "IR üretimi: Üye erişimi Load/Store implement edilmedi (LValue/RValue ve Borrowing gerekli)."); // Geçici mesaj
     return memberPtr; // Şimdilik sadece pointer'ı döndürelim
}


llvm::Value* IRGenerator::generateArrayAccessExpr(ast::ArrayAccessExprAST* expr) {
     // Taban ifadesi için IR üret (dizi, slice veya pointer)
     llvm::Value* base = generateExpr(expr->Base.get());
     if (!base) {
          Reporter.reportError(expr->loc, "IR üretimi: Dizi erişimi taban ifadesi için IR üretilemedi.");
          return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
     }

     // İndeks ifadesi için IR üret (tamsayı olmalı)
     llvm::Value* index = generateExpr(expr->Index.get());
      if (!index) {
          Reporter.reportError(expr->loc, "IR üretimi: Dizi erişimi indeks ifadesi için IR üretilemedi.");
          return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
      }

    // İndeksin tamsayı tipinde olduğundan emin ol (Sema yaptı)
    // Gerekirse indeksi i64'e cast et (GEP genellikle i64 indeks bekler)
    if (!index->getType()->isIntegerTy()) {
         Reporter.reportError(expr->Index->loc, "IR üretimi: Dizi indeksi tamsayı değil."); // Sema hatası olmalı
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
    }
    // İndeksi i64'e dönüştür
    index = Builder->CreateIntCast(index, llvm::Type::getInt64Ty(*Context), true, "index_cast"); // true: işaretli genişletme


    // Taban ifadesinin LLVM tipini al
    llvm::Type* baseLLVMType = base->getType();
    llvm::Value* basePtr = base; // Genellikle taban pointerdır

    llvm::Type* elementLLVMType = nullptr; // Dizi/Slice/Pointer'ın element tipi

    // Taban tipine göre GEP'in nasıl kullanılacağını belirle
    if (baseLLVMType->isPointerTy()) {
         // Taban bir pointer veya referans. İşaret ettiği tipi al.
         elementLLVMType = baseLLVMType->getPointerElementType();

         // GEP instruction'ı oluştur: GEP <BaseType>, <Pointer>, i64 <Index>
         // Array GEP ve Pointer GEP farklı sayıda indeks alabilir.
         // Dizi/Slice erişimi için genellikle <Pointer> ve i64 <Index> kullanılır.
         // Eğer base bir ArrayType* pointer'ı ise, ilk indeks 0 olmalıdır (dizinin başlangıcı).
         // Eğer base bir PointerType* pointer'ı ise, ilk indeks doğrudan element offset'idir.
         // Sema tarafından çözülen D++ tipini kullanarak ayrım yapın.

          sema::Type* baseDppType = expr->Base->resolvedType;
          if (baseDppType->isArrayType()) { // Array [T; N] veya Slice [T] pointer'ı
         //     // GEP <ElementType>, <ArrayPointer>, i64 0, i64 <Index>
         //     // ArrayType'ın kendisi { [N x T] } veya { T*, i64 } gibi bir struct olarak temsil ediliyorsa bu farklılaşır.
         //     // Varsayılan olarak Stack'teki [N x T] Array'ine pointer varsayalım.
               std::vector<llvm::Value*> indices = {
                   llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0), // İlk indeks 0 (array başlangıcı)
                   index // İkinci indeks eleman indeksi
               };
               memberPtr = Builder->CreateGEP(basePtr->getType()->getPointerElementType(), basePtr, indices, "array_element_ptr"); // Array Pointer GEP
         //
          } else if (baseDppType->isPointerType() || baseDppType->isReferenceType()) { // Pointer *T veya Referans &T
         //      // GEP <ElementType>, <Pointer>, i64 <Index>
               memberPtr = Builder->CreateGEP(basePtr->getType()->getPointerElementType(), basePtr, index, "pointer_element_ptr"); // Pointer GEP
          } else {
              Reporter.reportError(expr->Base->loc, "IR üretimi: Dizi erişimi için beklenmeyen pointer/referans tipi.");
              return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
          }
          Reporter.reportError(expr->loc, "IR üretimi: Dizi erişimi GEP implement edilmedi (Pointer/Array ayrımı gerekli)."); // Geçici mesaj
          return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata

     } else {
         // Taban bir değer ise, adresini almak gerekir. (LValue olmalı)
          if (!expr->Base->isLValue) { ... hata ... }
          basePtr = expr->Base->addressValue; // Örnek anotasyon
          baseLLVMType = basePtr->getType()->getPointerElementType(); // İşaret edilen tipi al (ArrayType veya StructType)
         // ... buradan sonra pointer/array case'i gibi devam eder ...
         Reporter.reportError(expr->Base->loc, "IR üretimi: Dizi erişimi tabanı pointer/referans değil (Karmaşık LValue'lar implement edilmedi).");
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
     }

    // Array erişimi ifadesinin kendisi, genellikle elemanın *değeridir* (Load instruction'ı gerekir).
    // Ancak atama sol tarafında (lvalue) ise Load gerekmez, GEP sonucu (pointer) kullanılır.
    // Sema tarafından belirlenen ifadenin lvalue/rvalue durumunu kullanın.
     if (expr->isLValue) { // Atama sol tarafı gibi kullanılıyor
         return memberPtr; // Pointer'ı döndür
     } else { // Normal kullanım (değer)
    //     // TODO: Eğer eleman mutable değilse ve dizi borrowed shared ise, bu bir Shared Borrow kullanımıdır.
    //     // checkBorrowRules / checkSymbolUsability
         return Builder->CreateLoad(elementLLVMType, memberPtr, "array_element_val"); // Load instruction
     }
     // Şimdilik sadece pointer'ı döndürelim
      Reporter.reportNote(expr->loc, "IR üretimi: Dizi erişimi Load/Store implement edilmedi (LValue/RValue ve Borrowing gerekli)."); // Geçici mesaj
      return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
}


llvm::Value* IRGenerator::generateNewExpr(ast::NewExprAST* expr) {
     // 'new' ifadesi, heap tahsisi ve constructor çağrısı gerektirir.
     // LLVM'de doğrudan 'new' instruction'ı yoktur. C çalışma zamanı kütüphanesindeki 'malloc' gibi fonksiyonları veya özel bir runtime allocator'ı çağırırız.

     // Oluşturulacak tipin LLVM tipini al
      sema::Type* targetDppType = expr->resolvedType; // Sema tarafından çözülen tip
      if (!isValidType(targetDppType)) { ... hata ... }
      llvm::Type* targetLLVMType = generateType(targetDppType);
      Reporter.reportNote(expr->loc, "IR üretimi: New ifadesi implement edilmedi."); // Geçici mesaj

      // TODO:
      // 1. Oluşturulacak tipin boyutunu hesapla (LLVM DataLayout kullanarak).
          llvm::DataLayout dataLayout = Module->getDataLayout();
          uint64_t typeSize = dataLayout.getTypeAllocSize(targetLLVMType);
      // 2. `malloc` fonksiyonunu bildir (ExternalLinkage) veya kendi allocator fonksiyonunuzu çağırın.
          llvm::Function* mallocFunc = Module->getFunction("malloc");
          if (!mallocFunc) { // Eğer bildirilmemişse
              llvm::Type* bytePtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*Context), 0);
              llvm::Type* sizeTy = llvm::Type::getInt64Ty(*Context); // sizeof sonucu genellikle size_t (platforma göre i64)
              llvm::FunctionType* mallocFTy = llvm::FunctionType::get(bytePtrTy, {sizeTy}, false);
              mallocFunc = llvm::Function::Create(mallocFTy, llvm::Function::ExternalLinkage, "malloc", *Module);
          }
      // 3. malloc'u çağır.
          llvm::Value* sizeConst = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), typeSize);
          llvm::Value* allocatedPtr = Builder->CreateCall(mallocFunc, {sizeConst}, "allocated_mem");
      // 4. allocatedPtr'ı hedef tipe cast et.
          llvm::Type* targetPtrLLVMType = llvm::PointerType::get(targetLLVMType, 0);
          llvm::Value* typedPtr = Builder->CreatePointerCast(allocatedPtr, targetPtrLLVMType, "typed_ptr");
      // 5. Constructor argümanları için IR üret ve constructor fonksiyonunu çağır (varsa).
          std::vector<llvm::Value*> argValues; // argümanlar için IR üret
      //    // Constructor fonksiyonunu bul (örn: sembol tablosundan veya TypeSystem'dan)
          Constructor ilk argüman olarak oluşturulan nesnenin pointer'ını alır.
      //    // Builder->CreateCall(constructorFunc, {typedPtr, argValues...}, "");
      // 6. Sonucu döndür (genellikle oluşturulan nesnenin pointer'ı veya D++ Box gibi bir wrapper).
          return typedPtr; // Veya bir Box<T> nesnesi oluşturup onu döndür

     return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata veya placeholder
}


llvm::Value* IRGenerator::generateMatchExpr(ast::MatchExprAST* expr) {
    // Match ifadesi, eşleşilen değere göre BasicBlock'lara dallanma (branching) üretir.
    // Karmaşık bir kontrol akışı yapısıdır ve Pattern Matching mantığını içerir.
    // LLVM'de switch veya iç içe if-else if yapıları ile temsil edilebilir.
    Reporter.reportNote(expr->loc, "IR üretimi: Match ifadesi implement edilmedi."); // Geçici mesaj
    // TODO:
    // 1. Eşleşilen ifade için IR üret.
        llvm::Value* matchedValue = generateExpr(expr->Value.get());
        llvm::Type* matchedLLVMType = matchedValue->getType();
    // 2. Her match kolu için BasicBlock oluştur (gövde ve opsiyonel guard).
    // 3. Tüm kolların sonunda birleşen bir Merge BasicBlock oluştur.
    // 4. Eşleşilen değere ve Pattern türlerine göre kontrol mantığı üret:
    //    - Literal pattern: Değer ile sabiti karşılaştır (ICmp, FCmp), CondBr ile kola dallan.
    //    - Identifier pattern: Genellikle bir binding, runtime kontrol gerektirmez.
    //    - Wildcard pattern (_): Her zaman eşleşir, koşulsuz olarak kola dallan.
    //    - Enum Variant pattern: Değer bir enum ise, discriminant (tag) alanını oku, beklenen varyantın tag değeriyle karşılaştır, CondBr ile kola dallan. Eğer varyant ilişkili veri içeriyorsa, Struct/Tuple deconstruction pattern gibi veriyi çıkar.
    //    - Struct Deconstruction pattern: Değer bir struct ise, ilgili alanlara GEP ile eriş, alanın değeri ile alt patterni (field binding) recursively match et. Birden fazla alan varsa, mantıksal AND ile birleştirilmiş CondBr dizisi gerekebilir.
    //    - Tuple pattern: Tuple elementlerine GEP ile eriş, her elementi karşılık gelen alt pattern ile recursively match et.
    //    - Reference pattern: Değer bir referans ise, dereference et (*), elde edilen değeri alt pattern ile match et.
    //    - Opsiyonel Guard (if condition): Pattern eşleştiyse, guard ifadesi için IR üret (bool olmalı), guard sonucu true ise kola dallan, false ise sonraki kola geç.
    // 5. Kontrol mantığı, ilk eşleşen pattern'ın gövde bloğuna CondBr veya Br ile dallanmalıdır.
    // 6. Her match kolu gövdesi için IR üret.
    // 7. Her match kolu gövdesinin sonundan Merge bloğuna dallan.
    // 8. Eğer match ifadesi bir değer döndürüyorsa (stmt değil expr), Merge bloğunda PHI düğümü kullanarak farklı kollardan gelen değerleri birleştir.
    //    Bu, tüm kolların aynı dönüş tipine sahip olmasını gerektirir (Sema kontrol etti).
    // 9. Insert point'i Merge bloğuna ayarla.

    return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata veya placeholder
}


llvm::Value* IRGenerator::generateBorrowExpr(ast::BorrowExprAST* expr) {
    // '&' veya '&mut' ifadesi, operandın *adresini* döndürür.
    // Operand Semantic Analyzer tarafından lvalue olarak doğrulandı ve borrow kuralları kontrol edildi.

    // Operand ifadesi için IR üret
    llvm::Value* operand = generateExpr(expr->Operand.get()); // Bu, operandın değerini değil, adresini üretmeli.
    // Örneğin, bir IdentifierExpr'in generateExpr'ı eğer lvalue ise Alloca* döndürmeli, rvalue ise Load instruction'ı döndürmeli.
    // Veya AST düğümlerine bir `generateAddress(expr)` metodu ekleyebilirsiniz.

    // Varsayalım generateExpr(expr->Operand.get()) eğer operand lvalue ise adresini, rvalue ise değerini döndürüyor.
    // Borrow işlemi için adres gerekir.
    // Eğer generateExpr değeri döndürüyorsa ve operand lvalue ise, adresini tekrar almalıyız.
    // Sema'nın AST düğümüne isLValue ve adresini veren bir `llvm::Value* addressValue;` alanı eklediğini varsayalım.
     if (expr->Operand->isLValue) {
    //    // Adresi kullan
        llvm::Value* operandAddress = expr->Operand->addressValue; // Örnek AST anotasyonu
    //    // TODO: Lifetime bilgisini döndürülen LLVM Referans (Pointer) tipine ekle (Çok karmaşık)
         Return operandAddress; // Borç alma işlemi adres döndürür
     } else {
         Reporter.reportError(expr->loc, "IR üretimi: '&' veya '&mut' rvalue üzerinde kullanılamaz."); // Sema hatası olmalı
         return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
     }
     Reporter.reportNote(expr->loc, "IR üretimi: Borrow ifadesi implement edilmedi (LValue ve Adres gerekli)."); // Geçici mesaj

     // Geçici olarak, operandın kendisi bir Alloca* ise onu döndürelim (değişken borrow'u)
      if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(expr->Operand.get())) {
          if (idExpr->resolvedSymbol && idExpr->resolvedSymbol->irValue) {
               return idExpr->resolvedSymbol->irValue; // Değişkenin Alloca* adresi
          }
      }
      return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata veya placeholder
}

llvm::Value* IRGenerator::generateDereferenceExpr(ast::DereferenceExprAST* expr) {
     // '*' ifadesi, pointer veya referansın işaret ettiği *değeri* döndürür.
     // Operand bir pointer tipi döndürmelidir (Sema kontrol etti).

     // Operand ifadesi için IR üret (Bu, pointer'ın/referansın kendisini veren Value*'ı üretmeli)
     llvm::Value* operandPointer = generateExpr(expr->Operand.get());
     if (!operandPointer || !operandPointer->getType()->isPointerTy()) {
          Reporter.reportError(expr->loc, "IR üretimi: Dereference operandı pointer tipi değil."); // Sema hatası veya dahili
          return llvm::UndefValue::get(generateType(expr->resolvedType)); // Hata
     }

    // İşaret edilen tipin LLVM tipini al
    llvm::Type* pointedToLLVMType = operandPointer->getType()->getPointerElementType();

    // Load instruction'ı oluştur. Bu, pointer'ın işaret ettiği bellek konumundaki değeri okur.
    llvm::Value* loadedValue = Builder->CreateLoad(pointedToLLVMType, operandPointer, "deref_val");

    // TODO: Sahiplik/Borrowing: Dereference sonucu immutable veya mutable lvalue olabilir.
    // Eğer operand mutable referans ise, sonuç mutable lvalue olmalıdır.
    // Eğer operand shared referans ise, sonuç immutable lvalue olmalıdır.
    // Eğer operand moved veya dropped ise, bu bir use-after-free hatasıdır (Sema yakaladı).
    // AST düğümüne lvalue/mutable lvalue bilgisi eklenebilir.
     expr->isLValue = true;
     expr->isMutable = operandExpr->resolvedType->isMutableReference(); // Örnek mutability kontrolü

    return loadedValue; // Yüklenen değeri döndür
}


// --- LLVM Yönetimi ve Yardımcıları Implementasyonları ---

// Yeni bir BasicBlock oluşturur ve bir fonksiyona ekler
llvm::BasicBlock* IRGenerator::createBasicBlock(const std::string& name, llvm::Function* func) {
    if (!func) {
         Reporter.reportError(SourceLocation(), "IR üretimi: BasicBlock oluşturulacak fonksiyon null.");
         // Hata durumu, null döndürelim veya istisna fırlatalım
         return nullptr;
    }
     // BasicBlock'u fonksiyonun sonuna ekler
    return llvm::BasicBlock::Create(*Context, name, func);
}

// IRBuilder'ın insert point'ini ayarlar
void IRGenerator::setInsertPoint(llvm::BasicBlock* block) {
    if (block) {
        Builder->SetInsertPoint(block);
    } else {
        // Insert point'i null yapmak, bir sonraki instruction'ın yerini belirleyemeyeceği anlamına gelir.
        // Bu, genellikle bir terminatör (Branch, Return) instruction'ı üretildikten sonra yapılır.
        Builder->ClearInsertionPoint();
    }
}

// Şu anki insert point'in bulunduğu BasicBlock'u döndürür
llvm::BasicBlock* IRGenerator::getCurrentBlock() const {
    return Builder->GetInsertBlock();
}

// Şu anki insert point'in bulunduğu fonksiyonu döndürür
llvm::Function* IRGenerator::getCurrentFunction() {
     // Builder'ın insert point'i bir BasicBlock içindeyse, o BasicBlock'un üst fonksiyonunu döndür.
     if (Builder->GetInsertBlock()) {
         return Builder->GetInsertBlock()->getParent();
     }
     // Aktif bir insert point yoksa, null döndür.
     return nullptr;
}


// Fonksiyonun giriş bloğuna bir değişken için stack alanı ayırır (Alloca)
llvm::AllocaInst* IRGenerator::createEntryBlockAlloca(llvm::Function* func, llvm::Type* varType, const std::string& varName, sema::Symbol* symbol) {
    if (!func || !varType) {
        Reporter.reportError(SourceLocation(), "IR üretimi: Alloca oluşturulacak fonksiyon veya tip null.");
        return nullptr;
    }
    // Geçici olarak Builder'ın insert point'ini fonksiyonun giriş bloğunun başına ayarla
    // Bu, Alloca'nın fonksiyonun başında tahsis edilmesini sağlar.
    llvm::IRBuilder<> TmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());

    // Alloca instruction'ı oluştur
    llvm::AllocaInst* alloca = TmpBuilder.CreateAlloca(varType, nullptr, varName.empty() ? "tmpalloca" : varName);

    // Eğer sembol verildiyse, sembolün irValue'sunu bu Alloca* olarak kaydet
     sema::Symbol struct'ına `llvm::Value* irValue;` alanı eklediğinizi varsayıyoruz.
     if (symbol) {
         symbol->irValue = alloca;
     }

    // Builder'ın insert point'ini eski yerine geri ayarla (generate... fonksiyonunun kaldığı yere)
    // Hayır, IRBuilder'ın insert point'i generate... fonksiyonları tarafından yönetilir ve createEntryBlockAlloca
    // çağrıldığında zaten o noktada olmalıdır. Alloca'nın giriş bloğuna gitmesi için geçici builder kullanılır.
    // Builder zaten doğru insert point'te olmalı, bu fonksiyon sadece geçici builder'ı kullanır.

    return alloca;
}


// Helper: Bir ifade sonucunu belirtilen LLVM tipine dönüştürür (gerekirse)
llvm::Value* IRGenerator::createTypeCast(llvm::Value* value, llvm::Type* targetType) {
    if (!value || !targetType) return nullptr;
    llvm::Type* currentType = value->getType();
    if (currentType == targetType) return value; // Zaten aynı tip

    // TODO: Semantik Analiz, hangi cast'lerin geçerli olduğunu belirledi.
    // Burada sadece o cast'e karşılık gelen LLVM instruction'ı üretilir.
    // Farklı cast türleri için LLVM Builder metodları kullanılır:
    // - Integer -> Integer (boyut değişimi): CreateIntCast (isSigned parametresi önemli)
    // - FloatingPoint -> FloatingPoint (boyut değişimi): CreateFPCast
    // - Integer -> FloatingPoint: CreateSIToFP (Signed), CreateUIToFP (Unsigned)
    // - FloatingPoint -> Integer: CreateFPToSI (Signed), CreateFPToUI (Unsigned)
    // - Pointer -> Pointer: CreatePointerCast
    // - Integer -> Pointer: CreateIntToPtr
    // - Pointer -> Integer: CreatePtrToInt
    // - Bitcast (aynı boyut, farklı yorum): CreateBitCast
    // - Struct/Array/Vector castleri? (Genellikle bitcast veya GEP ile erişim)

    // Basit bir örnek: Integer'dan Integer'a genişletme
    if (currentType->isIntegerTy() && targetType->isIntegerTy()) {
        // Kontrol et: Target tipi mevcut tipten daha geniş mi? İşaretli mi?
        // Sema, bunun geçerli bir cast olduğunu garanti etmeli.
        return Builder->CreateIntCast(value, targetType, true, "cast"); // Varsayılan olarak işaretli genişletme
    }
    // Basit bir örnek: Integer'dan Floating Point'e
     if (currentType->isIntegerTy() && targetType->isFloatingPointTy()) {
         // Sema, bunun geçerli olduğunu garanti etmeli (örn: int to float)
          return Builder->CreateSIToFP(value, targetType, "cast"); // İşaretli int to float
     }
     // Basit bir örnek: Floating Point'ten Integer'a
     if (currentType->isFloatingPointTy() && targetType->isIntegerTy()) {
          // Sema, bunun geçerli olduğunu garanti etmeli (örn: float to int)
           return Builder->CreateFPToSI(value, targetType, "cast"); // Float to işaretli int
     }


    // Desteklenmeyen veya anlamsız cast
    Reporter.reportError(SourceLocation(), "IR üretimi: Desteklenmeyen veya anlamsız tip dönüştürme (cast) üretilmeye çalışılıyor.");
    return llvm::UndefValue::get(targetType); // Hata durumunda hedef tipte bir undef değeri döndür
}


} // namespace irgen
} // namespace dppc
