#include "SemanticAnalyzer.h"
#include <stdexcept> // İstisnalar için
#include <sstream>   // String akımları için

// AST düğümlerinin spesifik tiplerini kullanmak için include'lar
#include "../parser/AST.h"

namespace dppc {
namespace sema {

// SemanticAnalyzer Constructor
SemanticAnalyzer::SemanticAnalyzer(ErrorReporter& reporter, SymbolTable& symbols, TypeSystem& types)
    : Reporter(reporter), Symbols(symbols), Types(types),
      CurrentScope(nullptr), CurrentFunctionReturnType(nullptr) // Başlangıçta kapsam ve dönüş tipi yok
{
    // Global kapsamı oluştur
    CurrentScope = Symbols.enterScope("global");

    // Ön tanımlı tipleri ve fonksiyonları sembol tablosuna ekle
    // Örneğin: int, float, bool, void tipleri
    declareSymbol("int", Symbol::SymbolKind::Type, Types.getIntType(), false, SourceLocation("<builtin>"));
    declareSymbol("float", Symbol::SymbolKind::Type, Types.getFloatType(), false, SourceLocation("<builtin>"));
    declareSymbol("bool", Symbol::SymbolKind::Type, Types.getBooleanType(), false, SourceLocation("<builtin>"));
    declareSymbol("void", Symbol::SymbolKind::Type, Types.getVoidType(), false, SourceLocation("<builtin>"));
    // D++'a özgü tipler
    // Option ve Result tiplerinin jenerik doğası burada basitçe temsil ediliyor,
    // TypeSystem'da daha sofistike bir jenerik tip temsili olmalıdır.
    declareSymbol("Option", Symbol::SymbolKind::Type, Types.getOptionType(nullptr), false, SourceLocation("<builtin>")); // Jenerikleşmemiş hali
    declareSymbol("Result", Symbol::SymbolKind::Type, Types.getResultType(nullptr, nullptr), false, SourceLocation("<builtin>")); // Jenerikleşmemiş hali

    // Println gibi temel fonksiyonlar
     void println(string s);
    std::vector<Type*> printlnParams = { Types.getStringType() };
    Type* printlnFuncType = Types.getFunctionType(Types.getVoidType(), printlnParams);
    declareSymbol("println", Symbol::SymbolKind::Function, printlnFuncType, false, SourceLocation("<builtin>"));

    // ... diğer ön tanımlı elemanları ekleyin
}

// Ana analiz fonksiyonu
bool SemanticAnalyzer::analyze(ast::SourceFileAST& root) {
    // Kök düğümden başlayarak bildirimleri analiz et
    for (const auto& decl : root.Declarations) {
        analyzeDecl(decl.get()); // unique_ptr'dan raw pointer al
    }

    // Global kapsamdan çık
    Symbols.exitScope();
    CurrentScope = nullptr; // Global kapsamdan çıkıldı

    // Hata raporlayıcıda hata var mı kontrol et
    return !Reporter.hasErrors();
}

// --- Genel Analiz Fonksiyonları ---

// Bildirimi analiz et
void SemanticAnalyzer::analyzeDecl(ast::DeclAST* decl) {
    if (!decl) return; // Geçersiz düğüm kontrolü

    // Bildirim türüne göre uygun analiz fonksiyonunu çağır
    if (auto funcDecl = dynamic_cast<ast::FunctionDeclAST*>(decl)) {
        analyzeFunctionDecl(funcDecl);
    } else if (auto structDecl = dynamic_cast<ast::StructDeclAST*>(decl)) {
        analyzeStructDecl(structDecl);
    } else if (auto traitDecl = dynamic_cast<ast::TraitDeclAST*>(decl)) {
        analyzeTraitDecl(traitDecl);
    } else if (auto implDecl = dynamic_cast<ast::ImplDeclAST*>(decl)) {
        analyzeImplDecl(implDecl);
    } else if (auto enumDecl = dynamic_cast<ast::EnumDeclAST*>(decl)) {
        analyzeEnumDecl(enumDecl);
    }
    // TODO: Diğer bildirim türleri (class, interface, union, alias, module)
    else {
        Reporter.reportError(decl->loc, "Bilinmeyen bildirim türü AST düğümü.");
    }
}

// Deyimi analiz et
void SemanticAnalyzer::analyzeStmt(ast::StmtAST* stmt) {
     if (!stmt) return;

    // Deyim türüne göre uygun analiz fonksiyonunu çağır
    if (auto blockStmt = dynamic_cast<ast::BlockStmtAST*>(stmt)) {
        analyzeBlockStmt(blockStmt);
    } else if (auto ifStmt = dynamic_cast<ast::IfStmtAST*>(stmt)) {
        analyzeIfStmt(ifStmt);
    } else if (auto whileStmt = dynamic_cast<ast::WhileStmtAST*>(stmt)) {
        analyzeWhileStmt(whileStmt);
    } else if (auto forStmt = dynamic_cast<ast::ForStmtAST*>(stmt)) {
        analyzeForStmt(forStmt);
    } else if (auto returnStmt = dynamic_cast<ast::ReturnStmtAST*>(stmt)) {
        analyzeReturnStmt(returnStmt);
    } else if (auto breakStmt = dynamic_cast<ast::BreakStmtAST*>(stmt)) {
        analyzeBreakStmt(breakStmt);
    } else if (auto continueStmt = dynamic_cast<ast::ContinueStmtAST*>(stmt)) {
        analyzeContinueStmt(continueStmt);
    } else if (auto letStmt = dynamic_cast<ast::LetStmtAST*>(stmt)) {
        analyzeLetStmt(letStmt);
    } else if (auto exprStmt = dynamic_cast<ast::ExprStmtAST*>(stmt)) {
         // Bir ifade deyimini analiz et
         analyzeExpr(exprStmt->Expr.get()); // İfadeyi analiz et, tipini görmezden gel (deyim olduğu için değeri kullanılmıyor)
         // Not: Bazı dillerde ifade deyiminin değeri 'void' olmalıdır gibi kurallar olabilir.
    }
    // TODO: Diğer deyim türleri (switch, do-while, foreach)
    else {
        Reporter.reportError(stmt->loc, "Bilinmeyen deyim türü AST düğümü.");
    }
}

// İfadeyi analiz et ve tipini döndür
Type* SemanticAnalyzer::analyzeExpr(ast::ExprAST* expr) {
     if (!expr) {
          Reporter.reportError(SourceLocation(), "Null ifade düğümü."); // Debug için
         return Types.getErrorType(); // Hata durumunda özel bir hata tipi döndür
     }

    // İfade türüne göre uygun analiz fonksiyonunu çağır
    Type* resolvedType = nullptr;
    if (auto intExpr = dynamic_cast<ast::IntegerExprAST*>(expr)) {
        resolvedType = analyzeIntegerExpr(intExpr);
    } else if (auto floatExpr = dynamic_cast<ast::FloatingExprAST*>(expr)) {
        resolvedType = analyzeFloatingExpr(floatExpr);
    } else if (auto stringExpr = dynamic_cast<ast::StringExprAST*>(expr)) {
        resolvedType = analyzeStringExpr(stringExpr);
    } else if (auto boolExpr = dynamic_cast<ast::BooleanExprAST*>(expr)) {
        resolvedType = analyzeBooleanExpr(boolExpr);
    } else if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(expr)) {
        resolvedType = analyzeIdentifierExpr(idExpr);
    } else if (auto binOpExpr = dynamic_cast<ast::BinaryOpExprAST*>(expr)) {
        resolvedType = analyzeBinaryOpExpr(binOpExpr);
    } else if (auto unOpExpr = dynamic_cast<ast::UnaryOpExprAST*>(expr)) {
         resolvedType = analyzeUnaryOpExpr(unOpExpr);
    } else if (auto callExpr = dynamic_cast<ast::CallExprAST*>(expr)) {
         resolvedType = analyzeCallExpr(callExpr);
    } else if (auto memberExpr = dynamic_cast<ast::MemberAccessExprAST*>(expr)) {
         resolvedType = analyzeMemberAccessExpr(memberExpr);
    } else if (auto arrayExpr = dynamic_cast<ast::ArrayAccessExprAST*>(expr)) {
         resolvedType = analyzeArrayAccessExpr(arrayExpr);
    } else if (auto newExpr = dynamic_cast<ast::NewExprAST*>(expr)) {
         resolvedType = analyzeNewExpr(newExpr);
    } else if (auto matchExpr = dynamic_cast<ast::MatchExprAST*>(expr)) {
         resolvedType = analyzeMatchExpr(matchExpr);
    } else if (auto borrowExpr = dynamic_cast<ast::BorrowExprAST*>(expr)) {
         resolvedType = analyzeBorrowExpr(borrowExpr);
    } else if (auto derefExpr = dynamic_cast<ast::DereferenceExprAST*>(expr)) {
         resolvedType = analyzeDereferenceExpr(derefExpr);
    }
    // TODO: Diğer ifade türleri (array literal, struct literal, tuple literal, lambda vb.)
    else {
        Reporter.reportError(expr->loc, "Bilinmeyen ifade türü AST düğümü.");
        resolvedType = Types.getErrorType(); // Hata durumunda
    }

    // Analiz edilen ifadenin tipini AST düğümüne ekle (AST düğümlerinde Type* tutan bir alan olmalı)
     expr->resolvedType = resolvedType; // ASTNode struct'ına resolvedType eklenmeli

    return resolvedType;
}

// TypeAST düğümünü analiz et ve çözümlenmiş Type* nesnesini döndür
Type* SemanticAnalyzer::analyzeType(ast::TypeAST* typeNode) {
     if (!typeNode) {
          Reporter.reportError(SourceLocation(), "Null tip düğümü."); // Debug için
         return Types.getErrorType(); // Hata durumunda özel bir hata tipi döndür
     }

     Type* resolvedType = nullptr;

     if (auto primType = dynamic_cast<ast::PrimitiveTypeAST*>(typeNode)) {
         // İlkel tipler ön tanımlıdır, TypeSystem'dan alınır
         switch (primType->TypeToken) {
             case lexer::TokenType::Kw_int: resolvedType = Types.getIntType(); break;
             case lexer::TokenType::Kw_uint: resolvedType = Types.getUintType(); break; // Eğer varsa
             case lexer::TokenType::Kw_short: resolvedType = Types.getShortType(); break; // Eğer varsa
             case lexer::TokenType::Kw_ushort: resolvedType = Types.getUshortType(); break; // Eğer varsa
             case lexer::TokenType::Kw_long: resolvedType = Types.getLongType(); break; // Eğer varsa
             case lexer::TokenType::Kw_ulong: resolvedType = Types.getUlongType(); break; // Eğer varsa
             case lexer::TokenType::Kw_float: resolvedType = Types.getFloatType(); break; // Float, double vb. için tek tip veya ayrı ayrı
             case lexer::TokenType::Kw_bool: resolvedType = Types.getBooleanType(); break;
             case lexer::TokenType::Kw_void: resolvedType = Types.getVoidType(); break;
             default:
                  Reporter.reportError(typeNode->loc, "Bilinmeyen ilkel tip tokenı.");
                  resolvedType = Types.getErrorType();
         }
     } else if (auto idType = dynamic_cast<ast::IdentifierTypeAST*>(typeNode)) {
         // Identifier tiplerini sembol tablosunda ara (Struct, Enum, Class, Alias, Trait vb.)
         if (auto symbol = lookupSymbol(idType->Name, idType->loc)) {
             if (symbol->kind == Symbol::SymbolKind::Type ||
                 symbol->kind == Symbol::SymbolKind::Struct ||
                 symbol->kind == Symbol::SymbolKind::Enum ||
                 symbol->kind == Symbol::SymbolKind::Class || // D'deki class
                 symbol->kind == Symbol::SymbolKind::Interface || // D'deki interface
                 symbol->kind == Symbol::SymbolKind::Trait) // D++'taki Trait
              {
                 resolvedType = symbol->type; // Sembolün temsil ettiği Type* nesnesini al
                 // TODO: Jenerik tipler için <...> argümanlarını burada parse edip TypeSystem'dan jenerik tipin instancelarını almanız gerekir.
             } else {
                 Reporter.reportError(idType->loc, "'" + idType->Name + "' bir tip adı değil.");
                 resolvedType = Types.getErrorType();
             }
         } else {
             Reporter.reportError(idType->loc, "Tanımlanmamış tip adı: '" + idType->Name + "'");
             resolvedType = Types.getErrorType();
         }
     } else if (auto ptrType = dynamic_cast<ast::PointerTypeAST*>(typeNode)) {
         // Pointer tip: Alt tipi analiz et
         Type* baseType = analyzeType(ptrType->BaseType.get());
         if (baseType && baseType != Types.getErrorType()) {
             resolvedType = Types.getPointerType(baseType);
         } else {
             resolvedType = Types.getErrorType(); // Alt tip hata verdiyse
         }
     } else if (auto refType = dynamic_cast<ast::ReferenceTypeAST*>(typeNode)) {
         // Referans tip: Alt tipi analiz et ve mutability bilgisini kullan
          Type* baseType = analyzeType(refType->BaseType.get());
          if (baseType && baseType != Types.getErrorType()) {
              // TypeSystem'dan ReferenceType* nesnesi alırken mutability bilgisini verin
              resolvedType = Types.getReferenceType(baseType, refType->IsMutable);
          } else {
              resolvedType = Types.getErrorType();
          }
     } else if (auto arrayType = dynamic_cast<ast::ArrayTypeAST*>(typeNode)) {
          // Dizi tip: Element tipini ve opsiyonel boyutu analiz et
         Type* elementType = analyzeType(arrayType->ElementType.get());
         // Boyut ifadesi (arrayType->Size.get()) compile-time sabit bir integer ifade olmalı.
         // analyzeExpr ile ifadeyi analiz et, sonra değerini compile-time'da değerlendir.
         long long arraySize = -1; // Boyut bilinmiyor veya hata
         if (arrayType->Size) {
              if (Type* sizeExprType = analyzeExpr(arrayType->Size.get())) {
                  // Boyut ifadesinin tamsayı olduğundan emin ol
                  if (sizeExprType == Types.getIntType() /* && compile-time evaluate edilebilir */) {
                      // TODO: Compile-time expression evaluation logic here
                      // Şimdilik sabiti doğrudan AST'den alalım (basitlik için, gerçekte ifade değerlendirilir)
                      if (auto intExpr = dynamic_cast<ast::IntegerExprAST*>(arrayType->Size.get())) {
                           arraySize = intExpr->Value;
                      } else {
                           Reporter.reportError(arrayType->Size->loc, "Dizi boyutu compile-time sabit tamsayı olmalıdır.");
                           resolvedType = Types.getErrorType();
                      }
                  } else {
                       Reporter.reportError(arrayType->Size->loc, "Dizi boyutu tamsayı olmalıdır.");
                       resolvedType = Types.getErrorType();
                  }
              } else {
                  resolvedType = Types.getErrorType(); // Boyut ifadesi analiz hatası
              }
         }

         if (elementType && elementType != Types.getErrorType() && resolvedType != Types.getErrorType()) {
             // TypeSystem'dan ArrayType* nesnesi al
             resolvedType = Types.getArrayType(elementType, arraySize);
         } else {
             resolvedType = Types.getErrorType(); // Element tipi veya boyut hata verdiyse
         }
     } else if (auto tupleType = dynamic_cast<ast::TupleTypeAST*>(typeNode)) {
         // Tuple tip: Element tiplerini analiz et
         std::vector<Type*> elementTypes;
         bool hadError = false;
         for (const auto& elemNode : tupleType->ElementTypes) {
             if (Type* elemType = analyzeType(elemNode.get())) {
                  if (elemType == Types.getErrorType()) hadError = true;
                  elementTypes.push_back(elemType);
             } else {
                  hadError = true;
                  elementTypes.push_back(Types.getErrorType()); // Yer tutucu
             }
         }
         if (!hadError) {
             resolvedType = Types.getTupleType(elementTypes);
         } else {
             resolvedType = Types.getErrorType();
         }
     } else if (auto optionType = dynamic_cast<ast::OptionTypeAST*>(typeNode)) {
         // Option<T> tip: İçteki T tipini analiz et
         Type* innerType = analyzeType(optionType->InnerType.get());
         if (innerType && innerType != Types.getErrorType()) {
              resolvedType = Types.getOptionType(innerType);
         } else {
             resolvedType = Types.getErrorType();
         }
     } else if (auto resultType = dynamic_cast<ast::ResultTypeAST*>(typeNode)) {
          // Result<T, E> tip: T ve E tiplerini analiz et
          Type* okType = analyzeType(resultType->OkType.get());
          Type* errType = analyzeType(resultType->ErrType.get());
          if (okType && okType != Types.getErrorType() && errType && errType != Types.getErrorType()) {
               resolvedType = Types.getResultType(okType, errType);
          } else {
              resolvedType = Types.getErrorType();
          }
     }
     // TODO: Diğer tip türleri (FunctionType, DelegateType, TraitObjectType, Generic instances)

     // Çözümlenmiş tipi TypeAST düğümüne ekle (AST düğümünde Type* tutan bir alan olmalı)
      typeNode->resolvedType = resolvedType; // ASTNode struct'ına resolvedType eklenmeli

     return resolvedType;
}

// Pattern'ı analiz et ve değişken bağlama
void SemanticAnalyzer::analyzePattern(ast::PatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope) {
     if (!pattern) return;

     // Pattern türüne göre analiz fonksiyonunu çağır
     if (auto idPattern = dynamic_cast<ast::IdentifierPatternAST*>(pattern)) {
         analyzeIdentifierPattern(idPattern, targetType, bindingScope);
     } else if (auto wildcardPattern = dynamic_cast<ast::WildcardPatternAST*>(pattern)) {
         analyzeWildcardPattern(wildcardPattern, targetType, bindingScope);
     } else if (auto literalPattern = dynamic_cast<ast::LiteralPatternAST*>(pattern)) {
         analyzeLiteralPattern(literalPattern, targetType, bindingScope);
     } else if (auto structPattern = dynamic_cast<ast::StructDeconstructionPatternAST*>(pattern)) {
         analyzeStructDeconstructionPattern(structPattern, targetType, bindingScope);
     } else if (auto enumPattern = dynamic_cast<ast::EnumVariantPatternAST*>(pattern)) {
         analyzeEnumVariantPattern(enumPattern, targetType, bindingScope);
     } else if (auto tuplePattern = dynamic_cast<ast::TuplePatternAST*>(pattern)) {
         analyzeTuplePattern(tuplePattern, targetType, bindingScope);
     } else if (auto refPattern = dynamic_cast<ast::ReferencePatternAST*>(pattern)) {
         analyzeReferencePattern(refPattern, targetType, bindingScope);
     }
     // TODO: analyzeRangePattern
     else {
          Reporter.reportError(pattern->loc, "Bilinmeyen desen (pattern) türü AST düğümü.");
     }
     // TODO: Pattern validity check (Desen eşleşilen tip ile uyumlu mu?)
      checkPatternValidity(pattern, targetType);
}


// --- Spesifik Bildirim Analiz Fonksiyonları ---

void SemanticAnalyzer::analyzeFunctionDecl(ast::FunctionDeclAST* funcDecl) {
    // Fonksiyon adını sembol tablosuna ekle
    // Geri dönüş tipini analiz et (şimdilik sadece AST düğümünden alalım, Type* çözümü daha sonra)
    Type* returnType = analyzeType(funcDecl->ReturnType.get());

    // Parametre tiplerini analiz et
    std::vector<Type*> paramTypes;
    for (const auto& param : funcDecl->Params) {
        if (Type* paramType = analyzeType(param->Type.get())) {
             paramTypes.push_back(paramType);
             // Parametre tipleri AST düğümlerine eklenmeli: param->Type->resolvedType = paramType;
        } else {
             // Hata raporlandı, hata tipi ekleyip devam et
             paramTypes.push_back(Types.getErrorType());
        }
    }

    // Fonksiyon tipini oluştur
    Type* functionType = Types.getFunctionType(returnType, paramTypes);

    // Fonksiyon sembolünü mevcut scope'a ekle
    // Fonksiyonlar varsayılan olarak mutable değildir (fonksiyon pointer'ı değiştiremezsiniz, ama içine mutable değişken alabilir)
    declareSymbol(funcDecl->Name, Symbol::SymbolKind::Function, functionType, false, funcDecl->loc);
    // TODO: Fonksiyon sembolüne parametre bilgisi (isim, tip) ve lokal değişkenler için pointer ekleyin.

    // Fonksiyon gövdesini analiz et (eğer varsa)
    if (funcDecl->Body) {
        // Fonksiyon gövdesi için yeni bir scope oluştur
        Symbols.enterScope("function: " + funcDecl->Name, CurrentScope);
        CurrentScope = Symbols.getCurrentScope(); // Kapsamı güncelle

        // Fonksiyonun dönüş tipini kaydet (return deyimini kontrol etmek için)
        CurrentFunctionReturnType = returnType;

        // Parametreleri bu yeni scope'a değişken olarak ekle
        for (const auto& param : funcDecl->Params) {
            // Parametrelerin mutability'si D++'ta nasıl? Varsayılan immutable mı? 'mut' anahtar kelimesiyle mutable mı?
            // Rust'ta varsayılan immutable'dır. 'mut' keyword ile mutable yapılır.
            bool isParamMutable = false; // TODO: ParameterAST struct'ına IsMutable alanı ekleyin ve parse edin
            declareSymbol(param->Name, Symbol::SymbolKind::Variable, getResolvedType(param->Type.get()), isParamMutable, param->loc);
            // Sembolün kaynağını (Declaration) AST düğümüne bağlamak faydalı olur: param->resolvedSymbol = symbol;
        }


        // Gövde içindeki deyimleri analiz et
        analyzeBlockStmt(funcDecl->Body.get());

        // Fonksiyon kapsamından çık
        CurrentScope = Symbols.exitScope(); // Kapsamı geri yükle
        CurrentFunctionReturnType = nullptr; // Dönüş tipini temizle
    }
}

void SemanticAnalyzer::analyzeStructDecl(ast::StructDeclAST* structDecl) {
    // Yapı (struct) adını sembol tablosuna ekle (Type kind olarak)
    // Şimdilik TypeSystem'dan bir Placeholder StructType* nesnesi alınabilir.
    // Alanlar analiz edildikten sonra TypeSystem'daki StructType güncellenebilir.
    Type* structType = Types.getStructType(structDecl->Name); // TypeSystem'da bir StructType* nesnesi oluştur/al

    // Yapı sembolünü mevcut scope'a ekle
    declareSymbol(structDecl->Name, Symbol::SymbolKind::Struct, structType, false, structDecl->loc);
    // TODO: Yapı sembolüne alan bilgileri ve metodlar için pointer ekleyin.

    // Yapının alanlarını analiz et
    std::vector<StructField> fields; // TypeSystem'daki StructType için alan listesi
    for (const auto& field : structDecl->Fields) {
        // Alanın tipini analiz et
        Type* fieldType = analyzeType(field->Type.get());
        if (fieldType && fieldType != Types.getErrorType()) {
             field->Type->resolvedType = fieldType;
             fields.emplace_back(field->Name, fieldType); // Alan bilgisini kaydet
        } else {
             // Hata raporlandı, hata tipiyle devam et
             fields.emplace_back(field->Name, Types.getErrorType());
        }
        // Alan adlarını aynı struct içinde benzersizliğini kontrol et
        // TODO: Duplicate field name check
    }

    // TypeSystem'daki StructType nesnesini alan bilgileriyle güncelle
    Types.updateStructType(structType, fields);

    // TODO: Yapıya ait metodları (eğer D sözdiziminde struct içinde metod tanımı varsa) analiz et.
    // D++'ta impl blokları kullanılabilir.
}

// Trait Bildirimi Analizi (D++'a özgü)
void SemanticAnalyzer::analyzeTraitDecl(ast::TraitDeclAST* traitDecl) {
     // Trait adını sembol tablosuna ekle (Trait kind olarak)
     Type* traitType = Types.getTraitType(traitDecl->Name); // TypeSystem'da bir TraitType* nesnesi oluştur/al
     declareSymbol(traitDecl->Name, Symbol::SymbolKind::Trait, traitType, false, traitDecl->loc);
     // TODO: Trait sembolüne gerektirdiği metodların bilgisi için pointer ekleyin.

     // Gerektirilen fonksiyon (metod) bildirimlerini analiz et
     // Bu fonksiyonların sadece imzaları vardır, gövdeleri yoktur.
     std::vector<TraitMethod> requiredMethods; // TypeSystem'daki TraitType için metod listesi
     for (const auto& func : traitDecl->RequiredFunctions) {
          // Fonksiyon adını, parametre tiplerini ve dönüş tipini analiz et
         Type* returnType = analyzeType(func->ReturnType.get());
         std::vector<Type*> paramTypes;
         bool paramError = false;
         for (const auto& param : func->Params) {
             if (Type* paramType = analyzeType(param->Type.get())) {
                 paramTypes.push_back(paramType);
                  Parametre tipleri AST düğümlerine eklenmeli: param->Type->resolvedType = paramType;
             } else {
                  paramError = true;
                  paramTypes.push_back(Types.getErrorType());
             }
         }
         if (!paramError && returnType && returnType != Types.getErrorType()) {
             Type* methodType = Types.getFunctionType(returnType, paramTypes);
             requiredMethods.emplace_back(func->Name, methodType); // Metod bilgisini kaydet
             // TODO: Metod adlarının aynı trait içinde benzersizliğini kontrol et.
         } else {
             Reporter.reportError(func->loc, "Trait metod bildirimi hatası: '" + func->Name + "'");
              requiredMethods.emplace_back(func->Name, Types.getErrorType());
         }

          // Trait metod bildirimlerinde gövde olmamalı
         if (func->Body) {
             Reporter.reportError(func->Body->loc, "Trait metod bildiriminde gövde olamaz.");
         }
     }
     // TypeSystem'daki TraitType nesnesini metod bilgileriyle güncelle
     Types.updateTraitType(traitType, requiredMethods);

     // TODO: Trait'lerin gerektirdiği ilişkili tipler veya sabitler olabilir, bunları da analiz et.
}

// Implementasyon (Impl) Bildirimi Analizi (D++'a özgü)
void SemanticAnalyzer::analyzeImplDecl(ast::ImplDeclAST* implDecl) {
     // Hangi tip için implementasyon yapıldığını analiz et
     // ImplDeclAST'de ForTypeName string olarak tutuluyor, bunu çözümlenmiş Type*'a çevir
     // Şimdilik TypeSystem'dan placeholder alalım
      Type* forType = analyzeType(implDecl->ForTypeNode.get()); // AST'de ForTypeNode varsa...
     // ImplDeclAST'de sadece ForTypeName stringi varsa, sembol tablosunda tip adını ara
     Type* forType = nullptr;
     if (auto symbol = lookupSymbol(implDecl->ForTypeName, implDecl->loc)) {
         if (symbol->kind == Symbol::SymbolKind::Type ||
             symbol->kind == Symbol::SymbolKind::Struct ||
             symbol->kind == Symbol::SymbolKind::Enum ||
             symbol->kind == Symbol::SymbolKind::Class) // İmplementasyon yapılacak tip türleri
          {
             forType = symbol->type;
         } else {
             Reporter.reportError(implDecl->loc, "Implementasyon ('impl') için tip adı bekleniyor, ancak '" + implDecl->ForTypeName + "' bir tip değil.");
              forType = Types.getErrorType();
         }
     } else {
         Reporter.reportError(implDecl->loc, "Implementasyon ('impl') için tanımlanmamış tip adı: '" + implDecl->ForTypeName + "'");
         forType = Types.getErrorType();
     }


     // Eğer bir trait implementasyonu ise, trait adını çözümle
     Type* traitType = nullptr;
     if (!implDecl->TraitName.empty()) {
         if (auto symbol = lookupSymbol(implDecl->TraitName, implDecl->loc)) {
             if (symbol->kind == Symbol::SymbolKind::Trait) {
                 traitType = symbol->type; // Çözümlenmiş TraitType*
             } else {
                 Reporter.reportError(implDecl->loc, "Implemente edilen ('impl') Trait adı bekleniyor, ancak '" + implDecl->TraitName + "' bir trait değil.");
                 traitType = Types.getErrorType();
             }
         } else {
             Reporter.reportError(implDecl->loc, "Implemente edilen ('impl') tanımlanmamış Trait adı: '" + implDecl->TraitName + "'");
             traitType = Types.getErrorType();
         }
     }

     // Implemente edilen metodları analiz et
     // Metodlar aslında bir Struct veya Class'ın üyeleri gibidir, ancak Impl bloğunda tanımlanırlar.
     // Bu metodlar için de FunctionDeclAST kullanılır, ancak bunlar üst düzey bildirimler değil, bir impl bloğunun üyeleridir.
     for (const auto& method : implDecl->Methods) {
         // Metod adını, parametrelerini ve dönüş tipini analiz et
          Type* returnType = analyzeType(method->ReturnType.get());
          std::vector<Type*> paramTypes;
          bool paramError = false;
          for (const auto& param : method->Params) {
              if (Type* paramType = analyzeType(param->Type.get())) {
                  paramTypes.push_back(paramType);
              } else {
                   paramError = true;
                   paramTypes.push_back(Types.getErrorType());
              }
          }
          if (!paramError && returnType && returnType != Types.getErrorType()) {
              Type* methodType = Types.getFunctionType(returnType, paramTypes);
              // TODO: Metod adlarının bu impl bloğu içinde veya ilgili tipin diğer impl blokları/kendi metodları arasında benzersizliğini kontrol et.
              // TODO: Eğer bu bir trait implementasyonu ise, metodun ilgili trait'teki bir metotla eşleşip eşleşmediğini (isim ve imza) kontrol et.

               // Metod gövdesini analiz et
              if (method->Body) {
                   // Metod gövdesi için yeni bir scope oluştur
                   Symbols.enterScope("method: " + method->Name + " for " + implDecl->ForTypeName, CurrentScope);
                   CurrentScope = Symbols.getCurrentScope();

                   // Metodun dönüş tipini kaydet
                   CurrentFunctionReturnType = returnType;

                   // Parametreleri scope'a ekle
                   for (const auto& param : method->Params) {
                        bool isParamMutable = false; // TODO: ParameterAST mutability
                        declareSymbol(param->Name, Symbol::SymbolKind::Variable, getResolvedType(param->Type.get()), isParamMutable, param->loc);
                   }

                   // Gövde içindeki deyimleri analiz et
                   analyzeBlockStmt(method->Body.get());

                   // Metod kapsamından çık
                   CurrentScope = Symbols.exitScope();
                   CurrentFunctionReturnType = nullptr;
              } else {
                   // Impl metodunun gövdesi olmalı
                   Reporter.reportError(method->loc, "Implementasyon ('impl') metodunun gövdesi olmalıdır.");
              }

          } else {
              Reporter.reportError(method->loc, "Implementasyon ('impl') metodu bildirimi hatası: '" + method->Name + "'");
          }
     }

     // TODO: TypeSystem'daki ForType'a bu Impl bloğunu bağlayın.
}

void SemanticAnalyzer::analyzeEnumDecl(ast::EnumDeclAST* enumDecl) {
    // Enum adını sembol tablosuna ekle (Type kind olarak)
    Type* enumType = Types.getEnumType(enumDecl->Name); // TypeSystem'da bir EnumType* nesnesi oluştur/al
    declareSymbol(enumDecl->Name, Symbol::SymbolKind::Enum, enumType, false, enumDecl->loc);
    // TODO: Enum sembolüne varyant bilgileri için pointer ekleyin.

    // Enum varyantlarını analiz et
    std::vector<EnumVariant> variants; // TypeSystem'daki EnumType için varyant listesi
    for (const auto& variant : enumDecl->Variants) {
        // Varyant adlarını aynı enum içinde benzersizliğini kontrol et
        // TODO: Duplicate variant name check
        variants.emplace_back(variant->Name); // Şimdilik sadece isim kaydediliyor
        // Eğer varyantların değerleri veya ilişkili tipleri varsa (D veya Rust gibi), bunları da analiz et.
         analyzeExpr(variant->Value.get()) veya analyzeType(variant->AssociatedTypes[i].get())
    }

    // TypeSystem'daki EnumType nesnesini varyant bilgileriyle güncelle
    Types.updateEnumType(enumType, variants);
}


// --- Spesifik Deyim Analiz Fonksiyonları ---

void SemanticAnalyzer::analyzeBlockStmt(ast::BlockStmtAST* blockStmt) {
    // Blok için yeni bir scope oluştur
    Symbols.enterScope("block", CurrentScope);
    CurrentScope = Symbols.getCurrentScope(); // Kapsamı güncelle

    // Blok içindeki deyimleri analiz et
    for (const auto& stmt : blockStmt->Statements) {
        analyzeStmt(stmt.get());
    }

    // Blok kapsamından çık
    CurrentScope = Symbols.exitScope(); // Kapsamı geri yükle
}

void SemanticAnalyzer::analyzeIfStmt(ast::IfStmtAST* ifStmt) {
    // Koşul ifadesini analiz et
    Type* conditionType = analyzeExpr(ifStmt->Condition.get());

    // Koşul ifadesinin bool tipinde olduğundan emin ol
    if (conditionType && conditionType != Types.getErrorType() && conditionType != Types.getBooleanType()) {
        Reporter.reportError(ifStmt->Condition->loc, "If koşulu 'bool' tipinde olmalıdır, ancak '" + conditionType->getName() + "' bulundu.");
    }

    // Then bloğunu analiz et (kendi scope'u içinde)
    analyzeBlockStmt(ifStmt->ThenBlock.get());

    // Opsiyonel Else bloğunu/If'ini analiz et (kendi scope'u içinde)
    if (ifStmt->ElseBlock) {
        analyzeStmt(ifStmt->ElseBlock.get()); // ElseIf de bir If deyimi olarak parse edilir
    }
}

void SemanticAnalyzer::analyzeWhileStmt(ast::WhileStmtAST* whileStmt) {
    // Koşul ifadesini analiz et
    Type* conditionType = analyzeExpr(whileStmt->Condition.get());

    // Koşul ifadesinin bool tipinde olduğundan emin ol
    if (conditionType && conditionType != Types.getErrorType() && conditionType != Types.getBooleanType()) {
        Reporter.reportError(whileStmt->Condition->loc, "While koşulu 'bool' tipinde olmalıdır, ancak '" + conditionType->getName() + "' bulundu.");
    }

    // Döngü gövdesini analiz et (kendi scope'u içinde)
    // Döngü içinde break/continue kullanılıp kullanılmadığını takip etmek için durum tutulabilir.
     bool wasInLoop = IsInLoop; IsInLoop = true;
    analyzeBlockStmt(whileStmt->Body.get());
     IsInLoop = wasInLoop; // Durumu geri yükle
}

void SemanticAnalyzer::analyzeForStmt(ast::ForStmtAST* forStmt) {
    // For döngüsü için yeni bir scope oluştur (init, condition, loopEnd bu scope'tadır)
    Symbols.enterScope("for-loop", CurrentScope);
    CurrentScope = Symbols.getCurrentScope();

    // Başlangıç deyimini analiz et (varsa)
    if (forStmt->Init) {
        analyzeStmt(forStmt->Init.get());
    }

    // Koşul ifadesini analiz et (varsa)
    if (forStmt->Condition) {
        Type* conditionType = analyzeExpr(forStmt->Condition.get());
        // Koşul ifadesinin bool tipinde olduğundan emin ol
        if (conditionType && conditionType != Types.getErrorType() && conditionType != Types.getBooleanType()) {
             Reporter.reportError(forStmt->Condition->loc, "For döngüsü koşulu 'bool' tipinde olmalıdır, ancak '" + conditionType->getName() + "' bulundu.");
        }
    }

    // Döngü gövdesini analiz et (kendi scope'u içinde, iç içe scope olabilir)
    // Döngü içinde break/continue kullanılıp kullanılmadığını takip etmek için durum tutulabilir.
     bool wasInLoop = IsInLoop; IsInLoop = true;
    analyzeBlockStmt(forStmt->Body.get());
     IsInLoop = wasInLoop; // Durumu geri yükle

    // Döngü sonu ifadesini analiz et (varsa)
    // Bu ifade genellikle bir deyimdir veya side effect'i olan bir ifadedir. Değeri kullanılmaz.
    if (forStmt->LoopEnd) {
        analyzeExpr(forStmt->LoopEnd.get());
    }

    // For döngüsü kapsamından çık
    CurrentScope = Symbols.exitScope();
}

void SemanticAnalyzer::analyzeReturnStmt(ast::ReturnStmtAST* returnStmt) {
    // Return deyimini sadece bir fonksiyonun içinde ise geçerli kabul et
    if (!CurrentFunctionReturnType) {
        Reporter.reportError(returnStmt->loc, "'return' deyimi sadece fonksiyon içinde kullanılabilir.");
        // Hata sonrası analizden çıkılabilir veya devam edilebilir
        return;
    }

    // Dönüş değerini analiz et (varsa)
    Type* returnedType = nullptr;
    if (returnStmt->ReturnValue) {
        returnedType = analyzeExpr(returnStmt->ReturnValue.get());
    } else {
        // Dönüş değeri yoksa, void tipini varsay
        returnedType = Types.getVoidType();
    }

    // Dönüş değerinin tipinin fonksiyonun dönüş tipi ile uyumlu olduğunu kontrol et
    if (returnedType && returnedType != Types.getErrorType()) {
        if (!isAssignable(CurrentFunctionReturnType, returnedType)) {
             Reporter.reportError(returnStmt->loc, "Fonksiyonun dönüş tipi '" + CurrentFunctionReturnType->getName() + "', ancak '" + returnedType->getName() + "' tipi döndürülüyor.");
        }
    }
}

void SemanticAnalyzer::analyzeBreakStmt(ast::BreakStmtAST* breakStmt) {
    // Break deyimini sadece bir döngü veya switch içinde ise geçerli kabul et
     if (!IsInLoop && !IsInSwitch) { // Durum değişkenleri
         Reporter.reportError(breakStmt->loc, "'break' deyimi sadece döngü veya switch içinde kullanılabilir.");
     }
     Reporter.reportError(breakStmt->loc, "'break' deyimi şu anda tam olarak kontrol edilmiyor (döngü/switch kapsamı)."); // Geçici mesaj
}

void SemanticAnalyzer::analyzeContinueStmt(ast::ContinueStmtAST* continueStmt) {
    // Continue deyimini sadece bir döngü içinde ise geçerli kabul et
     if (!IsInLoop) { // Durum değişkeni
         Reporter.reportError(continueStmt->loc, "'continue' deyimi sadece döngü içinde kullanılabilir.");
     }
    Reporter.reportError(continueStmt->loc, "'continue' deyimi şu anda tam olarak kontrol edilmiyor (döngü kapsamı)."); // Geçici mesaj
}


// Let deyimini analiz et (D++'a özgü)
void SemanticAnalyzer::analyzeLetStmt(ast::LetStmtAST* letStmt) {
    // Let deyiminin kendi scope'u olmayabilir, bulunduğu blok scope'unda değişkenleri bağlar.
    // Ancak pattern içindeki değişkenler için pattern analizi sırasında yeni, dar scope'lar gerekebilir.
    // Basitlik için şimdilik, let deyiminin bulunduğu scope'a değişkenleri bağlayalım.

    Type* initializerType = nullptr;
    if (letStmt->Initializer) {
        // Başlangıç değeri ifadesini analiz et
        initializerType = analyzeExpr(letStmt->Initializer.get());
    }

    Type* declaredType = nullptr;
    if (letStmt->Type) {
        // Belirtilen tipi analiz et
        declaredType = analyzeType(letStmt->Type.get());
    }

    // Pattern'ı analiz et
    // Pattern analizi sırasında, pattern'ın bağladığı değişkenler mevcut scope'a (veya pattern'ın kendi dar kapsamına) eklenir.
    // analyzePattern fonksiyonu, pattern'ın eşleşmesini beklenen tip (declaredType veya initializerType) ile kontrol etmeli
    // ve pattern içinde yeni değişkenler varsa bunları bindingScope'a eklemelidir.
    Type* targetTypeForPattern = declaredType ? declaredType : initializerType; // Pattern'ın eşleşmesi beklenen tip
    if (!targetTypeForPattern) {
        // Ne tip belirtilmiş ne de başlangıç değeri var: int x; gibi. D'de geçerli olabilir.
        // D++'ta let x; geçerli mi? Genellikle let x; veya let x = value; şeklindedir.
        Reporter.reportError(letStmt->loc, "'let' deyiminde tip belirtilmeli veya başlangıç değeri atanmalıdır.");
        targetTypeForPattern = Types.getErrorType(); // Hata tipiyle devam et
    }

    // Pattern'ı analiz et ve değişkenleri bağla. BindingScope olarak mevcut kapsamı verelim.
    // analyzePattern içinde, pattern'ın kendisi ve alt patternleri analiz edilir ve değişkenler bindingScope'a eklenir.
    analyzePattern(letStmt->Pattern.get(), targetTypeForPattern, CurrentScope);

    // Eğer hem tip belirtilmiş hem de başlangıç değeri atanmışsa, tiplerin uyumluluğunu kontrol et.
    if (declaredType && initializerType && declaredType != Types.getErrorType() && initializerType != Types.getErrorType()) {
        if (!isAssignable(declaredType, initializerType)) {
             Reporter.reportError(letStmt->Initializer->loc, "Başlangıç değeri ifadesinin tipi '" + initializerType->getName() + "', bildirilen tip '" + declaredType->getName() + "' ile uyumlu değil.");
        }
    }

    // TODO: Ownership ve mutability kurallarını kontrol et.
    // Let ile tanımlanan değişkenin sahipliği (ownership) olur.
    // Pattern içindeki değişkenlerin mutability'si `mut` ile belirtilir.
    // let mut x = 5; -> x mutable'dır.
    // let x = 5; -> x immutable'dır.
    // `analyzeIdentifierPattern` içinde Symbol tablosuna eklerken bu mutability'yi kaydet.

}

// --- Spesifik İfade Analiz Fonksiyonları ---

Type* SemanticAnalyzer::analyzeIntegerExpr(ast::IntegerExprAST* expr) {
    // Tam sayı literalinin tipi int'tir (veya uygun boyuttaki bir tamsayı tipi).
    // TODO: Literal değerine göre daha spesifik bir tip (byte, short, long vb.) seçilebilir.
     expr->resolvedType = Types.getIntType();
    return Types.getIntType();
}

Type* SemanticAnalyzer::analyzeFloatingExpr(ast::FloatingExprAST* expr) {
    // Kayan noktalı literalinin tipi float veya double'dır.
    // TODO: Literal değerine veya sonekine göre daha spesifik bir tip (float, double, real vb.) seçilebilir.
     expr->resolvedType = Types.getFloatType(); // float veya double için tek tip varsayalım
    return Types.getFloatType();
}

Type* SemanticAnalyzer::analyzeStringExpr(ast::StringExprAST* expr) {
    // String literalinin tipi string'tir.
     expr->resolvedType = Types.getStringType();
    return Types.getStringType();
}

Type* SemanticAnalyzer::analyzeBooleanExpr(ast::BooleanExprAST* expr) {
    // Boolean literalinin tipi bool'dur.
     expr->resolvedType = Types.getBooleanType();
    return Types.getBooleanType();
}

// Identifier (Değişken/Fonksiyon Adı) İfade Analizi
Type* SemanticAnalyzer::analyzeIdentifierExpr(ast::IdentifierExprAST* expr) {
    // Sembol tablosunda identifier adını ara
    if (auto symbol = lookupSymbol(expr->Name, expr->loc)) {
        // Sembol bulundu, ifadenin tipi sembolün tipidir.
         expr->resolvedType = symbol->type; // AST düğümüne çözümlenmiş tipi ekle
         expr->resolvedSymbol = symbol;   // AST düğümüne çözümlenmiş sembolü ekle

        // TODO: Sahiplik ve Ödünç Alma Kuralları Kontrolü
        // Değişkenin kullanım şekline göre (okuma, yazma, move, borrow) sembolün durumu kontrol edilmeli ve güncellenmeli.
        // Eğer sembol move edilmişse veya mutable borrow varsa ve sadece shared borrow bekleniyorsa hata ver.
         checkOwnershipRules(expr, symbol->type);

        return symbol->type; // Sembolün tipini döndür
    } else {
        // Sembol bulunamadı, tanımlanmamış identifier hatası ver
        Reporter.reportError(expr->loc, "Tanımlanmamış isim: '" + expr->Name + "'");
         expr->resolvedType = Types.getErrorType(); // AST düğümüne hata tipini ekle
         expr->resolvedSymbol = nullptr;          // Sembol yok
        return Types.getErrorType(); // Hata tipi döndür
    }
}

// İkili Operatör İfade Analizi
Type* SemanticAnalyzer::analyzeBinaryOpExpr(ast::BinaryOpExprAST* expr) {
    // Sol ve sağ operandları analiz et
    Type* leftType = analyzeExpr(expr->Left.get());
    Type* rightType = analyzeExpr(expr->Right.get());

    // Eğer operand tiplerinden herhangi biri hata tipiyse, sonuç da hata tipidir.
    if (leftType == Types.getErrorType() || rightType == Types.getErrorType()) {
         expr->resolvedType = Types.getErrorType();
        return Types.getErrorType();
    }

    // TODO: Operatörün operand tipleri için geçerli olup olmadığını kontrol et.
    // Örneğin, toplama (+) sayısal tipler için geçerlidir. Mantıksal AND (&&) boolean tipler için geçerlidir.
    // D'deki operatör aşırı yüklemeleri burada dikkate alınmalıdır.
    // Basit bir örnek: Aritmetik operatörler için operandlar aynı veya uyumlu sayısal tipler olmalı.

    Type* resultType = nullptr;
    switch (expr->Op) {
        case lexer::TokenType::Op_Plus:
        case lexer::TokenType::Op_Minus:
        case lexer::TokenType::Op_Star:
        case lexer::TokenType::Op_Slash:
        case lexer::TokenType::Op_Percent:
            // Sayısal tipler bekleniyor
            if (leftType->isNumericType() && rightType->isNumericType()) {
                 // Sonuç tipi operandların ortak tipi olacaktır (int + float -> float)
                 resultType = getCommonType(leftType, rightType, expr->loc);
            } else {
                 Reporter.reportError(expr->loc, "Aritmetik operatörler sayısal tipler için geçerlidir, ancak '" + leftType->getName() + "' ve '" + rightType->getName() + "' bulundu.");
                 resultType = Types.getErrorType();
            }
            break;
        case lexer::TokenType::Op_EqualEqual:
        case lexer::TokenType::Op_NotEqual:
        case lexer::TokenType::Op_LessThan:
        case lexer::TokenType::Op_GreaterThan:
        case lexer::TokenType::Op_LessEqual:
        case lexer::TokenType::Op_GreaterEqual:
            // Karşılaştırma operatörleri. Operandlar karşılaştırılabilir olmalı (genellikle aynı tip veya uyumlu sayısal). Sonuç bool'dur.
            if (leftType->isComparableWith(rightType)) { // TypeSystem'da isComparableWith metodu olmalı
                 resultType = Types.getBooleanType();
            } else {
                 Reporter.reportError(expr->loc, "Tipler karşılaştırılamaz: '" + leftType->getName() + "' ve '" + rightType->getName() + "'");
                 resultType = Types.getErrorType();
            }
            break;
        case lexer::TokenType::Op_And: // &&
        case lexer::TokenType::Op_Or:  // ||
             // Mantıksal operatörler. Operandlar bool olmalı. Sonuç bool'dur.
            if (leftType == Types.getBooleanType() && rightType == Types.getBooleanType()) {
                 resultType = Types.getBooleanType();
            } else {
                 Reporter.reportError(expr->loc, "Mantıksal operatörler 'bool' tipler için geçerlidir, ancak '" + leftType->getName() + "' ve '" + rightType->getName() + "' bulundu.");
                 resultType = Types.getErrorType();
            }
            break;
        case lexer::TokenType::Op_Equal: // Atama =
            // Sol operand atanabilir olmalı (lvalue), sağ operand sol tipine atanabilir olmalı. Sonuç sol operandın tipidir.
            // Atama operatörünün sonucu D'de sol operandın kendisidir, Rust'ta ise '()' (void) dir. D++ hangisini kullanıyor?
            // D standardını takip edelim: Atama sonucu sol operandın tipi.
            // Sol tarafın atanabilir (lvalue) olup olmadığını kontrol etmeniz gerekir.
            // Bu, analyzeExpr dönüş değeri Type* yerine, AST düğümüne bir 'isLValue' bayrağı eklenerek yapılabilir.
             if (expr->Left->isLValue) { // AST düğümüne isLValue eklendiğini varsayalım
                 if (isAssignable(leftType, rightType)) { // Sağ sol'a atanabilir mi kontrolü
                     resultType = leftType; // Sonuç tipi sol operandın tipi
                     // TODO: Sahiplik ve Ödünç Alma: Atama, sağ taraftaki değeri sola *move* eder (eğer sağ taraf move edilebilir bir değerse).
                     // Eğer sol bir borrow ise, sağ tarafın ömrünün borrow ömründen uzun olduğundan emin ol.
                      checkOwnershipRules(expr->Left.get(), leftType); // Sol tarafın kullanılabilirliğini kontrol et
                      checkOwnershipRules(expr->Right.get(), rightType); // Sağ tarafın move edilip edilmediğini kontrol et ve markala
                 } else {
                      Reporter.reportError(expr->loc, "Tip uyumsuzluğu: '" + rightType->getName() + "' tipi '" + leftType->getName() + "' tipine atanamaz.");
                      resultType = Types.getErrorType();
                 }
             } else {
                 Reporter.reportError(expr->Left->loc, "Atama sol tarafı atanabilir (lvalue) olmalıdır.");
                 resultType = Types.getErrorType();
             }
            break;
         // TODO: Diğer operatörler (Bitwise, Shift, Compound Assignment += vb., ., ->, ::)

        default:
            Reporter.reportError(expr->loc, "Bilinmeyen veya desteklenmeyen ikili operatör.");
            resultType = Types.getErrorType();
            break;
    }

     expr->resolvedType = resultType;
    return resultType;
}


// Tekli Operatör İfade Analizi
Type* SemanticAnalyzer::analyzeUnaryOpExpr(ast::UnaryOpExprAST* expr) {
    Type* operandType = analyzeExpr(expr->Operand.get());

    if (operandType == Types.getErrorType()) {
         expr->resolvedType = Types.getErrorType();
        return Types.getErrorType();
    }

    Type* resultType = nullptr;
    switch (expr->Op) {
        case lexer::TokenType::Op_Minus: // Tekli eksi (-)
            // Operand sayısal olmalı, sonuç operand ile aynı tipte.
            if (operandType->isNumericType()) {
                resultType = operandType;
            } else {
                Reporter.reportError(expr->loc, "Tekli '-' operatörü sayısal tipler için geçerlidir, ancak '" + operandType->getName() + "' bulundu.");
                 resultType = Types.getErrorType();
            }
            break;
        case lexer::TokenType::Op_Not: // Mantıksal NOT (!)
            // Operand bool olmalı, sonuç bool.
             if (operandType == Types.getBooleanType()) {
                 resultType = Types.getBooleanType();
            } else {
                 Reporter.reportError(expr->loc, "Tekli '!' operatörü 'bool' tipi için geçerlidir, ancak '" + operandType->getName() + "' bulundu.");
                 resultType = Types.getErrorType();
            }
            break;
        case lexer::TokenType::Op_Star: // Dereference (*)
             // Operand bir pointer veya referans olmalı. Sonuç, işaret edilen tipin değeridir (rvalue).
             // analyzeDereferenceExpr içinde daha detaylı kontrol edilecek.
             // Buradan analyzeDereferenceExpr'ı çağırmak veya logic'i oraya taşımak daha iyi.
             // DereferenceExprAST'nin aslında bir UnaryOpExprAST olmaması, ayrı bir düğüm olması daha doğrudur.
             // AST yapısını bu şekilde güncelliyorsanız, bu case'i kaldırın.
             if (auto derefExpr = dynamic_cast<ast::DereferenceExprAST*>(expr)) { // Eğer DereferenceExprAST'den kalıtım alıyorsa
                  resultType = analyzeDereferenceExpr(derefExpr);
             } else { // Eğer hala genel UnaryOpExprAST ise ve Op_Star ise...
                 // TODO: check if operandType is PointerType or ReferenceType
                 // If operandType is PointerType p, result is the base type *p
                 // If operandType is ReferenceType r, result is the base type &r
                 if (auto ptrType = dynamic_cast<PointerType*>(operandType)) {
                      resultType = ptrType->getBaseType();
                 } else if (auto refType = dynamic_cast<ReferenceType*>(operandType)) {
                      resultType = refType->getBaseType();
                 } else {
                      Reporter.reportError(expr->loc, "Tekli '*' operatörü pointer veya referans tipler için geçerlidir, ancak '" + operandType->getName() + "' bulundu.");
                      resultType = Types.getErrorType();
                 }
                 // TODO: Sahiplik ve Ödünç Alma: Dereference, referansın ömrü içinde veriye erişim sağlar.
                 // Eğer referans mutable ise, sonuç da mutable bir referans gibi davranabilir (lvalue).
                  checkDereferenceRules(expr, operandType); // Kontrol kuralları
             }
            break;
        case lexer::TokenType::Op_BitAnd: // Ödünç Alma (&)
             // Operand bir lvalue olmalı (değer değil). Sonuç referans tipidir.
             // analyzeBorrowExpr içinde daha detaylı kontrol edilecek.
             // Eğer BorrowExprAST'den kalıtım alıyorsa, logic'i oraya taşıyın.
             // Eğer hala genel UnaryOpExprAST ise ve Op_BitAnd ise...
              if (auto borrowExpr = dynamic_cast<ast::BorrowExprAST*>(expr)) { // Eğer BorrowExprAST'den kalıtım alıyorsa
                   resultType = analyzeBorrowExpr(borrowExpr);
              } else { // Eğer hala genel UnaryOpExprAST ise ve Op_BitAnd ise...
                  // TODO: check if operandExpr is LValue
                   bool isMutableBorrow = false; // 'mut' keyword'ü burada nasıl işleniyor? Lexer/Parser UnaryOpExprAST'ye bilgi verdi mi?
                  // Result is ReferenceType(operandType, isMutableBorrow)
                  // If operand is mutable variable/field, can take &mut. If immutable, only &.
                  // If operand is already mutably borrowed, cannot take another & or &mut.
                  // If operand is already immutably borrowed, cannot take &mut. Can take another &.
                    checkBorrowRules(expr, operandType); // Kontrol kuralları
                  Reporter.reportError(expr->loc, "Tekli '&' operatörü (ödünç alma) şu anda tam olarak kontrol edilmiyor."); // Geçici mesaj
                   resultType = Types.getReferenceType(operandType, false); // Varsayılan olarak immutable referans tipi döndür
              }
            break;

        default:
             Reporter.reportError(expr->loc, "Bilinmeyen veya desteklenmeyen tekli operatör.");
             resultType = Types.getErrorType();
             break;
    }

     expr->resolvedType = resultType;
    return resultType;
}

// Fonksiyon Çağrısı Analizi
Type* SemanticAnalyzer::analyzeCallExpr(ast::CallExprAST* expr) {
     // Çağrılan ifadeyi analiz et (şimdilik sadece Identifier olduğunu varsayalım)
     // İleride, çağrılan ifade bir fonksiyon pointer'ı, struct/class metodu veya closure olabilir.
      analyzeExpr(expr->CalleeExpr.get()); // Eğer AST'de CalleeExpr olsaydı

     // Sembol tablosunda fonksiyon adını ara (şimdilik CalleeName stringini kullanıyoruz)
     if (auto funcSymbol = lookupSymbol(expr->CalleeName, expr->loc)) {
         // Sembol bulundu, fonksiyon kind olduğundan emin ol
         if (funcSymbol->kind == Symbol::SymbolKind::Function) {
             Type* functionType = funcSymbol->type;
             if (auto funcSig = dynamic_cast<FunctionType*>(functionType)) { // Sembolün bir fonksiyon tipi olduğundan emin ol
                 // Argümanları analiz et
                 std::vector<Type*> argTypes;
                 bool argError = false;
                 for (const auto& arg : expr->Args) {
                     if (Type* argType = analyzeExpr(arg.get())) {
                          argTypes.push_back(argType);
                          // TODO: Sahiplik ve Ödünç Alma: Argümanlar fonksiyonlara genellikle move veya borrow ile geçer.
                           checkOwnershipRules(arg.get(), argType); // Argümanın move edilip edilmediğini kontrol et
                     } else {
                          argError = true;
                          argTypes.push_back(Types.getErrorType());
                     }
                 }

                 // Argüman sayısı ve tiplerinin fonksiyon imzası ile eşleştiğini kontrol et
                 if (!argError && argTypes.size() == funcSig->getParameterTypes().size()) {
                     bool typesMatch = true;
                     for (size_t i = 0; i < argTypes.size(); ++i) {
                         if (!isAssignable(funcSig->getParameterTypes()[i], argTypes[i])) { // Argüman parametreye atanabilir mi?
                             Reporter.reportError(expr->Args[i]->loc, "Argüman tipi '" + argTypes[i]->getName() + "', parametre tipi '" + funcSig->getParameterTypes()[i]->getName() + "' ile uyumlu değil.");
                             typesMatch = false;
                         }
                     }
                     if (typesMatch) {
                          // Fonksiyon çağrısı başarılı, sonucun tipi fonksiyonun dönüş tipidir.
                          expr->resolvedType = funcSig->getReturnType();
                          expr->resolvedFunction = funcSymbol; // AST düğümüne çözümlenmiş fonksiyon sembolünü ekle
                         return funcSig->getReturnType();
                     } else {
                         // Tipler eşleşmedi, hata raporlandı
                          expr->resolvedType = Types.getErrorType();
                          expr->resolvedFunction = funcSymbol; // Hata olsa da sembolü ekleyebiliriz
                         return Types.getErrorType();
                     }
                 } else {
                     // Argüman sayısı eşleşmedi
                     Reporter.reportError(expr->loc, "Fonksiyon '" + expr->CalleeName + "' için beklenen argüman sayısı " + std::to_string(funcSig->getParameterTypes().size()) + ", ancak " + std::to_string(argTypes.size()) + " sağlandı.");
                      expr->resolvedType = Types.getErrorType();
                      expr->resolvedFunction = funcSymbol;
                     return Types.getErrorType();
                 }

             } else {
                 // Sembol bulundu ama tipi fonksiyon değil
                 Reporter.reportError(expr->loc, "'" + expr->CalleeName + "' bir fonksiyon değil.");
                  expr->resolvedType = Types.getErrorType();
                  expr->resolvedFunction = funcSymbol;
                 return Types.getErrorType();
             }
         } else {
              // Sembol bulundu ama kind'ı fonksiyon değil
             Reporter.reportError(expr->loc, "'" + expr->CalleeName + "' bir fonksiyon değil.");
               expr->resolvedType = Types.getErrorType();
               expr->resolvedFunction = funcSymbol;
             return Types.getErrorType();
         }

     } else {
         // Fonksiyon sembolü bulunamadı, tanımlanmamış fonksiyon hatası
         Reporter.reportError(expr->loc, "Tanımlanmamış fonksiyon: '" + expr->CalleeName + "'");
          expr->resolvedType = Types.getErrorType();
          expr->resolvedFunction = nullptr;
         return Types.getErrorType();
     }
}


// Üye Erişimi Analizi (. veya ->)
Type* SemanticAnalyzer::analyzeMemberAccessExpr(ast::MemberAccessExprAST* expr) {
    // Taban ifadesini analiz et
    Type* baseType = analyzeExpr(expr->Base.get());

    if (baseType == Types.getErrorType()) {
         expr->resolvedType = Types.getErrorType();
        return Types.getErrorType();
    }

    Type* resolvedMemberType = nullptr;
    bool memberFound = false;

    // Taban tipi bir pointer veya referans ise, önce dereference yapılması gerekebilir
    Type* actualBaseType = baseType;
    if (expr->IsPointerAccess) { // -> operatörü kullanıldıysa, taban kesin pointer olmalı
        if (auto ptrType = dynamic_cast<PointerType*>(baseType)) {
            actualBaseType = ptrType->getBaseType();
        } else {
            Reporter.reportError(expr->Base->loc, "'->' operatörü sadece pointer tipler için geçerlidir, ancak '" + baseType->getName() + "' bulundu.");
              expr->resolvedType = Types.getErrorType();
            return Types.getErrorType();
        }
    }
    // Eğer '.' operatörü kullanıldıysa ve taban bir referans ise, implicit dereference yapılabilir (Rust gibi)
    else if (auto refType = dynamic_cast<ReferenceType*>(baseType)) {
         // Implicit dereference yapılıyor
         actualBaseType = refType->getBaseType();
         // TODO: Referansın ömrü ve borrow kurallarını kontrol et. Üye erişimi shared veya mutable borrow oluşturur.
         // checkBorrowRules for member access
    }


    // Taban tipi bir struct, class veya union olmalı
    if (auto structType = dynamic_cast<StructType*>(actualBaseType)) {
         // Struct alanlarını ara
         if (auto field = structType->getField(expr->MemberName)) { // TypeSystem'da getField metodu olmalı
             resolvedMemberType = field->type; // Alanın tipini al
             memberFound = true;
             // TODO: Field sembolünü AST düğümüne bağla: expr->resolvedField = field;
             // TODO: Sahiplik ve Ödünç Alma: Alan erişimi, ana nesnenin sahiplik/borrow durumunu etkiler.
             // Eğer ana nesne borrowed shared ise, alan da shared borrowed olur. Eğer ana nesne owned ise, alan copied/moved olabilir.
             // checkOwnershipRules / checkBorrowRules for member access result.
         } else {
             Reporter.reportError(expr->loc, "Yapı '" + structType->getName() + "' içinde '" + expr->MemberName + "' adında bir alan bulunamadı.");
              resolvedMemberType = Types.getErrorType();
         }
    } else if (auto classType = dynamic_cast<ClassType*>(actualBaseType)) { // D'deki class
         // Class üyelerini ara (alanlar, metodlar)
         // TODO: Lookup member in ClassType (inheritance, access modifiers public/private/protected)
         Reporter.reportError(expr->loc, "Class üye erişimi analizi henüz implement edilmedi."); // Geçici mesaj
         resolvedMemberType = Types.getErrorType();

    } else if (auto unionType = dynamic_cast<UnionType*>(actualBaseType)) { // D'deki union
         // Union üyelerini ara
         // TODO: Lookup member in UnionType (semantics of union access)
         Reporter.reportError(expr->loc, "Union üye erişimi analizi henüz implement edilmedi."); // Geçici mesaj
         resolvedMemberType = Types.getErrorType();
    }
    // TODO: Trait object member access (trait metodları)
    // TODO: Enum member access (Enum.Variant veya Enum.MemberValue)


    if (!memberFound && resolvedMemberType != Types.getErrorType()) {
         Reporter.reportError(expr->loc, "Üye erişimi için geçerli bir tip ('" + actualBaseType->getName() + "') beklenen yapı, sınıf veya union değil.");
         resolvedMemberType = Types.getErrorType();
    }


     expr->resolvedType = resolvedMemberType;
    return resolvedMemberType;
}

// Dizi Erişimi Analizi ([])
Type* SemanticAnalyzer::analyzeArrayAccessExpr(ast::ArrayAccessExprAST* expr) {
     // Taban ifadesini analiz et
     Type* baseType = analyzeExpr(expr->Base.get());

     if (baseType == Types.getErrorType()) {
          expr->resolvedType = Types.getErrorType();
         return Types.getErrorType();
     }

     // İndeks ifadesini analiz et
     Type* indexType = analyzeExpr(expr->Index.get());

     if (indexType == Types.getErrorType()) {
          expr->resolvedType = Types.getErrorType();
         return Types.getErrorType();
     }

     // Taban tipi bir dizi, slice veya pointer olmalı
     Type* elementType = nullptr;
     bool isValidBase = false;

     if (auto arrayType = dynamic_cast<ArrayType*>(baseType)) {
         elementType = arrayType->getElementType();
         isValidBase = true;
     } else if (auto sliceType = dynamic_cast<SliceType*>(baseType)) { // Slice tipi varsa
          elementType = sliceType->getElementType();
          isValidBase = true;
     } else if (auto pointerType = dynamic_cast<PointerType*>(baseType)) { // Pointer üzerinden dizi erişimi (C-stili)
          elementType = pointerType->getBaseType();
          isValidBase = true;
     } else {
          Reporter.reportError(expr->Base->loc, "Dizi erişimi için geçerli bir tip ('" + baseType->getName() + "') bekleniyor (dizi, slice veya pointer).");
     }


     // İndeks tipi tamsayı olmalı
     if (indexType && indexType != Types.getErrorType() && !indexType->isIntegerType()) { // TypeSystem'da isIntegerType metodu olmalı
         Reporter.reportError(expr->Index->loc, "Dizi indeksi tamsayı tipinde olmalıdır, ancak '" + indexType->getName() + "' bulundu.");
     }

     // Eğer temel tip ve indeks tipi geçerliyse, sonucun tipi element tipidir.
     Type* resultType = nullptr;
     if (isValidBase && (indexType && indexType->isIntegerType()) && elementType && elementType != Types.getErrorType()) {
         // Dizi erişiminin sonucu, element tipinin bir referansı veya kopyası olabilir. D++'ta genellikle referanstır.
         // Eğer dizi/slice mutable ise, sonuç &mut ElementType'dir. Değilse &ElementType.
         // checkOwnershipRules / checkBorrowRules for array access result.
         bool isBaseMutable = false; // TODO: Taban ifadenin mutable olup olmadığını belirle
         resultType = Types.getReferenceType(elementType, isBaseMutable); // Varsayılan olarak referans döndürsün
     } else {
          resultType = Types.getErrorType();
     }

     expr->resolvedType = resultType;
    return resultType;
}

// new ifadesini analiz et
Type* SemanticAnalyzer::analyzeNewExpr(ast::NewExprAST* expr) {
    // Oluşturulacak tip adını çözümle
     Type* targetType = nullptr;
     if (auto symbol = lookupSymbol(expr->TypeName, expr->loc)) {
         if (symbol->kind == Symbol::SymbolKind::Type ||
             symbol->kind == Symbol::SymbolKind::Struct ||
             symbol->kind == Symbol::SymbolKind::Class) // new ile oluşturulabilecek tip türleri
          {
             targetType = symbol->type;
         } else {
             Reporter.reportError(expr->loc, "'new' ile oluşturulan beklenen bir tip adı, ancak '" + expr->TypeName + "' bir tip değil.");
             targetType = Types.getErrorType();
         }
     } else {
         Reporter.reportError(expr->loc, "'new' ile oluşturulan tanımlanmamış tip adı: '" + expr->TypeName + "'");
         targetType = Types.getErrorType();
     }

     // Constructor argümanlarını analiz et
     std::vector<Type*> argTypes;
     bool argError = false;
     for (const auto& arg : expr->Args) {
         if (Type* argType = analyzeExpr(arg.get())) {
              argTypes.push_back(argType);
         } else {
              argError = true;
              argTypes.push_back(Types.getErrorType());
         }
     }

     // TODO: Uygun constructor'ı bul ve argüman tipleriyle eşleştiğini kontrol et.
     // Struct veya Class tiplerinin constructor bilgileri TypeSystem veya ilgili sembolde saklanmalıdır.
     if (targetType && targetType != Types.getErrorType() && !argError) {
          // Constructor eşleştiğini varsayalım.
          // 'new' ifadesinin sonucu genellikle oluşturulan tipin bir pointer'ıdır (C++ gibi)
          // veya D++'ta sahipliği olan bir boxed değer olabilir (Rust'taki Box<T> gibi).
          // D'de 'new' object'in kendisini döndürür, pointer değil.
          // D++'ta 'new' ne döndürüyor? Sahip olunan bir değer mi (Box<T> benzeri) yoksa pointer mı?
          // Varsayılan olarak sahip olunan bir değer (owned T) döndürdüğünü varsayalım (Rust Box gibi).
          // TypeSystem'da BoxType<T> gibi bir temsil olabilir. Veya basitçe T tipinde owned bir değer.
          // Eğer pointer döndürüyorsa: resultType = Types.getPointerType(targetType);

          // D++'ta 'new' ile heap tahsisi ve sahiplik destekleniyorsa, sonuç owned bir değer olmalı.
           expr->resolvedType = targetType; // Sonuç tipi, oluşturulan tipin kendisidir (owned).
          return targetType; // Veya Box<targetType> gibi bir tip döndürün
     } else {
           expr->resolvedType = Types.getErrorType();
          return Types.getErrorType();
     }
}

// Match İfadesi Analizi (D++'a özgü)
Type* SemanticAnalyzer::analyzeMatchExpr(ast::MatchExprAST* expr) {
    // Eşleşilen ifadeyi analiz et
    Type* matchedType = analyzeExpr(expr->Value.get());

    if (matchedType == Types.getErrorType()) {
         expr->resolvedType = Types.getErrorType();
        return Types.getErrorType();
    }

    // Match kollarını analiz et
    Type* resultType = nullptr; // Tüm kolların ortak dönüş tipi
    bool firstArm = true;
    bool hadError = false;

    for (auto& arm : expr->Arms) {
        // Her match kolu için yeni bir scope oluştur (pattern içinde bağlanan değişkenler için)
        Symbols.enterScope("match-arm", CurrentScope);
        CurrentScope = Symbols.getCurrentScope(); // Kapsamı güncelle

        // Kolun desenini (pattern) analiz et
        // analyzePattern, pattern'ın eşleşilen tip (matchedType) ile uyumlu olduğunu kontrol etmeli
        // ve pattern içindeki değişkenleri CurrentScope'a bağlamalıdır.
        analyzePattern(arm.Pattern.get(), matchedType, CurrentScope);

        // Opsiyonel guard ifadesini analiz et (varsa)
        if (arm.Guard) {
            Type* guardType = analyzeExpr(arm.Guard.get());
            // Guard ifadesinin bool tipinde olduğundan emin ol
            if (guardType && guardType != Types.getErrorType() && guardType != Types.getBooleanType()) {
                Reporter.reportError(arm.Guard->loc, "Match guard ifadesi 'bool' tipinde olmalıdır, ancak '" + guardType->getName() + "' bulundu.");
                hadError = true;
            }
        }

        // Kolun gövdesini (ifadesini) analiz et
        Type* armBodyType = analyzeExpr(arm.Body.get());

        if (armBodyType == Types.getErrorType()) {
            hadError = true;
        }

        // İlk kolun tipi, match ifadesinin varsayılan sonuç tipidir.
        if (firstArm) {
            resultType = armBodyType;
            firstArm = false;
        } else {
            // Sonraki kolların tipinin ilk kolun tipi ile uyumlu olduğunu kontrol et.
            // Tüm kolların aynı tipte olması veya ortak bir tipe dönüştürülebilir olması gerekir.
            if (resultType && armBodyType && resultType != Types.getErrorType() && armBodyType != Types.getErrorType()) {
                 // Ortak bir tip bulmaya çalış
                 Type* common = getCommonType(resultType, armBodyType, arm.Body->loc);
                 if (!common) {
                     Reporter.reportError(arm.Body->loc, "Match kolu tipi '" + armBodyType->getName() + "', önceki kolların tipi '" + resultType->getName() + "' ile uyumlu değil.");
                     hadError = true;
                 } else {
                     resultType = common; // Ortak tipi yeni sonuç tipi yap
                 }
            } else {
                 hadError = true; // Önceki kollar veya bu kol hata verdiyse
            }
        }

        // Match kolu kapsamından çık
        CurrentScope = Symbols.exitScope(); // Kapsamı geri yükle
    }

    // TODO: Pattern Exhaustiveness (Tüm olası durumlar match ediliyor mu?) kontrolü.
     checkPatternExhaustiveness(expr, matchedType); // Oldukça karmaşık bir kontrol

    if (hadError) {
         expr->resolvedType = Types.getErrorType();
        return Types.getErrorType();
    } else {
         expr->resolvedType = resultType;
        return resultType; // Tüm kolların ortak tipi
    }
}


// Ödünç Alma (Borrow) İfade Analizi (&, &mut)
Type* SemanticAnalyzer::analyzeBorrowExpr(ast::BorrowExprAST* expr) {
     // Operand ifadesini analiz et
     Type* operandType = analyzeExpr(expr->Operand.get());

     if (operandType == Types.getErrorType()) {
          expr->resolvedType = Types.getErrorType();
         return Types.getErrorType();
     }

     // Operand bir lvalue olmalı (değer değil)
     // AST düğümünde isLValue bayrağı olduğunu varsayalım.
     if (!expr->Operand->isLValue) { // TODO: AST düğümüne isLValue bayrağı ekleyin ve parse/analyzeExpr sırasında ayarlayın.
         Reporter.reportError(expr->Operand->loc, "Ödünç alma (& veya &mut) için atanabilir (lvalue) ifade bekleniyor.");
           expr->resolvedType = Types.getErrorType();
         return Types.getErrorType();
     }

     // TODO: Sahiplik ve Ödünç Alma Kuralları Kontrolü
     // Operandın mevcut durumunu (Owned, BorrowedShared, BorrowedMutable, Moved) kontrol et.
     // Eğer '&mut' ise:
     // - Operand mutable olmalı.
     // - Operand şu anda başka bir shared veya mutable borrow tarafından kullanılmıyor olmalı.
     // Eğer '&' ise:
     // - Operand şu anda başka bir mutable borrow tarafından kullanılmıyor olmalı.
     // - Operand mutable veya immutable olabilir.
      checkBorrowRules(expr, operandType); // checkBorrowRules içinde Symbol tablosundaki durumu güncelleyin

     // Sonuç tipi bir referans tipidir.
     Type* resultType = Types.getReferenceType(operandType, expr->IsMutable);

     // TODO: Yaşam süresi analizi. Oluşturulan referansın yaşam süresi, referansın işaret ettiği verinin (operandın) yaşam süresi ile sınırlı olmalıdır.
      analyzeLifetimes(expr); // Bu fonksiyonun çağrılması ve lifetime'ları ilişkilendirmesi gerekir.
     // Result type should be ReferenceType with an associated lifetime: &lifetime mut? Type

      expr->resolvedType = resultType; // AST düğümüne referans tipini (lifetime ile) ekle
     return resultType; // Şimdilik lifetime olmadan tipi döndürelim
}


// Dereference İfade Analizi (*)
Type* SemanticAnalyzer::analyzeDereferenceExpr(ast::DereferenceExprAST* expr) {
    // Operand ifadesini analiz et
    Type* operandType = analyzeExpr(expr->Operand.get());

    if (operandType == Types.getErrorType()) {
         expr->resolvedType = Types.getErrorType();
        return Types.getErrorType();
    }

    // Operand bir pointer veya referans olmalı
    Type* resultType = nullptr;
    Type* baseType = nullptr;
    bool isMutableReference = false; // Dereference sonucu mutable bir lvalue mu?

    if (auto ptrType = dynamic_cast<PointerType*>(operandType)) {
        baseType = ptrType->getBaseType();
        // Pointer dereference sonucu genellikle atanabilir (lvalue) olabilir.
         checkDereferenceRules(expr, operandType); // Pointer dereference kuralları
         resultType = baseType; // Sonuç tipi işaret edilen tip
          expr->resolvedType = resultType;
          expr->isLValue = true; // Sonuç lvalue
        return resultType;

    } else if (auto refType = dynamic_cast<ReferenceType*>(operandType)) {
         baseType = refType->getBaseType();
         isMutableReference = refType->isMutable();
         // Referans dereference sonucu atanabilir (lvalue) olabilir ve mutability referansın mutability'sine bağlıdır.
           checkDereferenceRules(expr, operandType); // Referans dereference kuralları
          resultType = baseType; // Sonuç tipi işaret edilen tip
           expr->resolvedType = resultType;
           expr->isLValue = true; // Sonuç lvalue
           expr->isMutable = isMutableReference; // Sonuç mutable mi?
         return resultType;

    } else {
        Reporter.reportError(expr->loc, "Dereference '*' operatörü pointer veya referans tipler için geçerlidir, ancak '" + operandType->getName() + "' bulundu.");
         expr->resolvedType = Types.getErrorType();
        return Types.getErrorType();
    }
}


// --- Pattern Analiz Fonksiyonları (analyzePattern tarafından çağrılır) ---

void SemanticAnalyzer::analyzeWildcardPattern(ast::WildcardPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope) {
    // Wildcard pattern (_) herhangi bir tip ile eşleşir ve hiçbir değişken bağlamaz. Anlamsal kontrol basittir.
     pattern->resolvedType = targetType; // Hangi tip ile eşleştiğini işaretle
}

void SemanticAnalyzer::analyzeIdentifierPattern(ast::IdentifierPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope) {
    // Identifier pattern (x, mut x) hedef tip ile eşleşir ve yeni bir değişken bağlar.
    // Değişkeni bindingScope'a ekle.
    if (!targetType || targetType == Types.getErrorType()) {
         Reporter.reportError(pattern->loc, "Identifier deseninin eşleştiği geçerli bir hedef tipi yok.");
          pattern->resolvedType = Types.getErrorType();
         return;
    }

    // Yeni sembolü bindingScope'a ekle
    // IdentifierPattern içindeki IsMutable bayrağını kullanarak değişkenin mutability'sini belirle.
    // checkPatternValidity içinde pattern'ın hedef tip ile uyumluğu kontrol edilebilir (örn: void tipe değişken bağlayamazsınız).
    declareSymbol(pattern->Name, Symbol::SymbolKind::Variable, targetType, pattern->IsMutable, pattern->loc);
    // Eklenen sembolü AST düğümüne bağla: pattern->resolvedSymbol = symbol;
     pattern->resolvedType = targetType; // Hangi tip ile eşleştiğini işaretle
}

void SemanticAnalyzer::analyzeLiteralPattern(ast::LiteralPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope) {
    // Literal pattern, sadece değeri literal ile aynı ve tipi hedef tip ile uyumlu ise eşleşir.
    // Literal ifadesini analiz et ve tipini al
    Type* literalType = analyzeExpr(pattern->Literal.get());

     if (!literalType || literalType == Types.getErrorType() || !targetType || targetType == Types.getErrorType()) {
         // Hata zaten raporlandı
          pattern->resolvedType = Types.getErrorType();
         return;
     }

    // Literal tipinin hedef tipe atanabilir (veya eşit) olduğunu kontrol et.
    if (!isAssignable(targetType, literalType)) { // Literal değeri hedef tipe atanabilir mi?
        Reporter.reportError(pattern->loc, "Literal deseninin tipi '" + literalType->getName() + "', eşleşilen tip '" + targetType->getName() + "' ile uyumlu değil.");
          pattern->resolvedType = Types.getErrorType();
         return;
    }
     pattern->resolvedType = targetType; // Hangi tip ile eşleştiğini işaretle
}

void SemanticAnalyzer::analyzeStructDeconstructionPattern(ast::StructDeconstructionPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope) {
     // Struct deconstruction pattern, sadece hedef tip bir struct ise ve alanlar eşleşiyorsa geçerlidir.
    if (auto structType = dynamic_cast<StructType*>(targetType)) {
         // Pattern'daki struct adı ile hedef tipin struct adı eşleşiyor mu kontrol et? AST'de TypeName var.
          if (pattern->TypeName != structType->getName()) { ... hata ... }

         // Alan eşleşmelerini analiz et. Her alan eşleşmesi kendi içinde bir pattern'dır.
         for (const auto& fieldMatch : pattern->Fields) {
             // Struct tipinde bu isimde bir alan var mı kontrol et
             if (auto field = structType->getField(fieldMatch.FieldName)) {
                  // Alan bulundu. Alanın tipini al ve bu tipi alt pattern için hedef tip olarak kullan.
                  analyzePattern(fieldMatch.Binding.get(), field->type, bindingScope); // Alt patterni analiz et ve değişkenleri bağla
             } else {
                 Reporter.reportError(pattern->loc, "Yapı '" + structType->getName() + "' içinde '" + fieldMatch.FieldName + "' adında bir alan bulunamadı.");
                 // Hata sonrası toparlanma düşünülebilir
             }
         }
         // TODO: ignoreExtraFields (..) varsa, bu alana karşılık gelen herhangi bir pattern olmadığını doğrula.
           pattern->resolvedType = targetType;
    } else {
        Reporter.reportError(pattern->loc, "Yapı deconstruction deseni sadece yapı (struct) tipleri için geçerlidir, ancak '" + targetType->getName() + "' bulundu.");
          pattern->resolvedType = Types.getErrorType();
    }
}

void SemanticAnalyzer::analyzeEnumVariantPattern(ast::EnumVariantPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope) {
     // Enum variant pattern, sadece hedef tip bir enum ise ve varyant eşleşiyorsa geçerlidir.
     if (auto enumType = dynamic_cast<EnumType*>(targetType)) {
         // Pattern'daki enum adı ile hedef tipin enum adı eşleşiyor mu kontrol et? AST'de EnumName var.
           if (pattern->EnumName != enumType->getName()) { ... hata ... }

         // Enum tipinde bu isimde bir varyant var mı kontrol et.
         if (auto variant = enumType->getVariant(pattern->VariantName)) { // TypeSystem'da getVariant metodu olmalı
              // Varyant bulundu. Varyantın ilişkili tipleri varsa, pattern argümanlarının bu tiplerle eşleştiğini kontrol et.
              // TODO: Eğer varyantın ilişkili tipleri varsa (Rust gibi tuple/struct variant), pattern argümanlarını analiz et.
               analyzePattern(pattern->Args[i].get(), variant->getAssociatedType(i), bindingScope);

              if (!pattern->Args.empty()) { // Eğer pattern'ın argümanları varsa
                   // TODO: Varyantın ilişkili tipleri olduğundan emin ol ve sayılarını karşılaştır.
                  Reporter.reportError(pattern->loc, "Enum varyantı '" + pattern->VariantName + "' ilişkili tip/alan almaz, ancak pattern argümanları sağlandı.");
                  // Hata sonrası toparlanma
              }
              pattern->resolvedType = targetType;
         } else {
              Reporter.reportError(pattern->loc, "Enum '" + enumType->getName() + "' içinde '" + pattern->VariantName + "' adında bir varyant bulunamadı.");
               pattern->resolvedType = Types.getErrorType();
         }
     } else {
         Reporter.reportError(pattern->loc, "Enum varyant deseni sadece enum tipleri için geçerlidir, ancak '" + targetType->getName() + "' bulundu.");
          // pattern->resolvedType = Types.getErrorType();
     }
}

void SemanticAnalyzer::analyzeTuplePattern(ast::TuplePatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope) {
    // Tuple pattern, sadece hedef tip bir tuple ise ve element sayısı eşleşiyorsa geçerlidir.
     if (auto tupleType = dynamic_cast<TupleType*>(targetType)) {
          // Pattern'daki element sayısı ile hedef tipin element sayısı eşleşiyor mu kontrol et.
         if (pattern->Elements.size() != tupleType->getElementTypes().size()) {
             Reporter.reportError(pattern->loc, "Tuple desenindeki element sayısı (" + std::to_string(pattern->Elements.size()) + "), eşleşilen tuple tipinin element sayısı (" + std::to_string(tupleType->getElementTypes().size()) + ") ile eşleşmiyor.");
             // Hata sonrası toparlanma düşünülebilir
         } else {
             // Her bir element patternini ilgili tuple elementi tipiyle analiz et.
             for (size_t i = 0; i < pattern->Elements.size(); ++i) {
                 analyzePattern(pattern->Elements[i].get(), tupleType->getElementTypes()[i], bindingScope);
             }
         }
          pattern->resolvedType = targetType;
     } else {
         Reporter.reportError(pattern->loc, "Tuple deseni sadece tuple tipleri için geçerlidir, ancak '" + targetType->getName() + "' bulundu.");
           pattern->resolvedType = Types.getErrorType();
     }
}

void SemanticAnalyzer::analyzeReferencePattern(ast::ReferencePatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope) {
    // Referans pattern, sadece hedef tip bir referans ise geçerlidir.
     if (auto refType = dynamic_cast<ReferenceType*>(targetType)) {
         // Referansın mutability'si pattern'ın mutability'si ile uyumlu olmalı.
          if (pattern->IsMutable && !refType->isMutable()) {
              Reporter.reportError(pattern->loc, "Mutable referans deseni (&mut), immutable referans tipine ('" + targetType->getName() + "') uygulanamaz.");
          }

         // Alt patterni referansın işaret ettiği tip ile analiz et.
         analyzePattern(pattern->SubPattern.get(), refType->getBaseType(), bindingScope);
           pattern->resolvedType = targetType;
     } else {
         Reporter.reportError(pattern->loc, "Referans deseni (& veya &mut) sadece referans tipleri için geçerlidir, ancak '" + targetType->getName() + "' bulundu.");
           pattern->resolvedType = Types.getErrorType();
     }
}


// --- Yardımcı Fonksiyonlar İmplementasyonları ---

// Sembol tablosunda ismi ara
SymbolTable::Symbol* SemanticAnalyzer::lookupSymbol(const std::string& name, const SourceLocation& loc) {
    if (auto symbol = Symbols.lookup(name)) {
        return symbol;
    }
    // Bulunamadıysa hata raporlama lookupSymbol içinde yapılabilir veya burada yapılır.
     Reporter.reportError(loc, "Tanımlanmamış isim: '" + name + "'");
    return nullptr; // Bulunamadı
}

// Sembol tablosuna sembol ekle
void SemanticAnalyzer::declareSymbol(const std::string& name, Symbol::SymbolKind kind, Type* type, bool isMutable, const SourceLocation& loc) {
    // Aynı scope içinde aynı isimde başka bir sembol var mı kontrol et
    if (auto existingSymbol = Symbols.lookupInCurrentScope(name)) { // SymbolTable'da bu metod olmalı
        Reporter.reportError(loc, "'" + name + "' ismi zaten bu kapsamda tanımlanmış.");
        Reporter.reportNote(existingSymbol->loc, "Önceki tanım burada yapıldı.");
    } else {
        // Yeni sembolü ekle
        Symbols.addSymbol(name, kind, type, isMutable, loc); // SymbolTable'da bu metod olmalı
    }
}

// TypeAST düğümünden çözümlenmiş Type* nesnesini al
Type* SemanticAnalyzer::getResolvedType(ast::TypeAST* typeNode) {
     // analyzeType fonksiyonu TypeAST düğümünü analiz eder ve Type* döndürür.
     // AST düğümüne resolvedType eklendiğinde, bu fonksiyonda sadece düğümdeki resolvedType alanını döndürebilirsiniz.
     return analyzeType(typeNode); // Şimdilik her seferinde analiz etsin
      return typeNode->resolvedType; // AST düğümünde resolvedType alanı varsa
}


// İki tipin ortak paydasını bul (örn: int ve float'ın ortak paydası float'tır)
Type* SemanticAnalyzer::getCommonType(Type* type1, Type* type2, const SourceLocation& loc) {
    if (!type1 || !type2 || type1 == Types.getErrorType() || type2 == Types.getErrorType()) {
        return Types.getErrorType();
    }
    if (type1 == type2) {
        return type1;
    }

    // TODO: Tip hiyerarşisine ve implicit dönüşüm kurallarına göre ortak tipi belirle.
    // Örneğin:
    if (type1->isNumericType() && type2->isNumericType()) {
        // Sayısal tipler için geniş olan tipi döndür (int < float < double vb.)
        // Bu logic TypeSystem içinde olmalıdır.
         return Types.getWiderNumericType(type1, type2);
        Reporter.reportError(loc, "Ortak tip belirleme mantığı henüz tam implement edilmedi: '" + type1->getName() + "' ve '" + type2->getName() + "'"); // Geçici mesaj
         return Types.getErrorType(); // Şimdilik uyumsuz varsayalım
    }

    // Eğer tipler arasında implicit dönüşüm yoksa ve aynı değillerse, ortak payda yoktur (genellikle)
    Reporter.reportError(loc, "Tiplerin ortak bir paydası yok: '" + type1->getName() + "' ve '" + type2->getName() + "'");
    return Types.getErrorType(); // Uyumsuz
}


// srcType, destType'a atanabilir mi?
bool SemanticAnalyzer::isAssignable(Type* destType, Type* srcType) {
     if (!destType || !srcType || destType == Types.getErrorType() || srcType == Types.getErrorType()) {
        return false; // Hata tipleri atanabilir değildir
     }
     if (destType == srcType) {
         return true; // Aynı tipler atanabilir
     }

     // TODO: Atanabilirlik kurallarını implement et (implicit dönüşümler, subtyping, array/slice atanabilirliği, pointer atanabilirliği vb.)
     // Örneğin:
     if (destType->isNumericType() && srcType->isNumericType()) {
          // Küçük sayısal tipten büyük sayısal tipe atama genellikle geçerlidir.
           return Types.isNumericAssignable(destType, srcType); // TypeSystem'da bu metod olmalı
          return true; // Şimdilik sayısal tipleri birbirine atanabilir varsayalım (basitlik için)
     }

     // Pointer atanabilirliği (*T -> *const T geçerli olabilir)
     // Referans atanabilirliği (&mut T -> &T geçerli olabilir)
     // Option/Result atanabilirliği

     // Eğer özel bir kural yoksa ve tipler aynı değilse, atanabilir değildir.
     // Reporter.reportNote(SourceLocation(), "Atanabilirlik kontrolü henüz tam implement edilmedi: '" + srcType->getName() + "' -> '" + destType->getName() + "'"); // Debug için
     return false; // Varsayılan olarak atanabilir değil
}

// fromType, toType'a dönüştürülebilir mi? (isAssignable'dan farklı olarak explicit cast'ler için)
bool SemanticAnalyzer::isConvertible(Type* fromType, Type* toType) {
     if (!fromType || !toType || fromType == Types.getErrorType() || toType == Types.getErrorType()) {
        return false; // Hata tipleri dönüştürülebilir değildir
     }
     if (fromType == toType) {
         return true; // Aynı tipler dönüştürülebilir (boş dönüşüm)
     }

     // TODO: Explicit dönüşüm (cast) kurallarını implement et.
     // Örneğin: Sayısal tipler arası dönüşüm, pointer <-> integer, pointer <-> pointer (reinterpret_cast benzeri)
     // Casting için isAssignable'dan daha gevşek kurallar olabilir.

     Reporter.reportError(SourceLocation(), "Dönüştürülebilirlik (cast) kontrolü henüz tam implement edilmedi: '" + fromType->getName() + "' -> '" + toType->getName() + "'"); // Geçici mesaj
     return false; // Varsayılan olarak dönüştürülebilir değil
}


// --- D++'a Özgü Analiz Yardımcıları (Placeholder'lar) ---

void SemanticAnalyzer::checkOwnershipRules(ast::ExprAST* expr, Type* exprType) {
     // Bu fonksiyon, bir ifadenin sonucunun nasıl kullanıldığını (move edildi mi, kopyalandı mı, borrow edildi mi)
     // ve bu kullanımın sahiplik kurallarına uyup uymadığını kontrol eder.
     // Sembol tablosundaki ilgili değişkenin durumunu günceller.
     // Örneğin, bir owned değeri move ettikten sonra tekrar kullanmak hatadır.
     // İfadenin kendisinin (expr) bir değişkene mi, alan erişimine mi vb. karşılık geldiğini belirlemek gerekir.
     Reporter.reportNote(expr->loc, "Sahiplik kuralları kontrolü (checkOwnershipRules) implement edilmedi."); // Geçici mesaj
}

void SemanticAnalyzer::checkBorrowRules(ast::BorrowExprAST* borrowExpr, Type* operandType) {
     // Bu fonksiyon, '&' veya '&mut' ile yapılan ödünç alma işleminin geçerli olup olmadığını kontrol eder.
     // Operandın mutable'lığı, mevcut borrow'lar ve operandın yaşam süresi gibi faktörleri dikkate alır.
     // Sembol tablosundaki ilgili değişkenin borrow sayısını ve durumunu günceller.
     Reporter.reportNote(borrowExpr->loc, "Ödünç alma kuralları kontrolü (checkBorrowRules) implement edilmedi."); // Geçici mesaj
}

void SemanticAnalyzer::checkDereferenceRules(ast::DereferenceExprAST* derefExpr, Type* operandType) {
     // Bu fonksiyon, '*' ile yapılan dereference işleminin geçerli olup olmadığını kontrol eder.
     // Operandın pointer veya referans olduğundan ve null olmadığına dair (bazı dillerde) kontrol yapılabilir.
     // Sahiplik/Borrowing: Dereference, referansın/pointer'ın işaret ettiği değere erişim sağlar.
     // checkOwnershipRules veya checkBorrowRules burayı çağırabilir.
     Reporter.reportNote(derefExpr->loc, "Dereference kuralları kontrolü (checkDereferenceRules) implement edilmedi."); // Geçici mesaj
}

void SemanticAnalyzer::checkMutability(ast::ExprAST* targetExpr, bool isAssignment) {
     // Bu fonksiyon, bir değişkene, alana veya referansa yazma (atama gibi) veya mutable erişim yapılıp yapılmadığını kontrol eder.
     // Eğer targetExpr mutable değilse (immutable değişken, immutable referans vb.) hata verir.
     // targetExpr'ın bir lvalue olduğundan emin olunur.
     Reporter.reportNote(targetExpr->loc, "Mutability kontrolü (checkMutability) implement edilmedi."); // Geçici mesaj
}

void SemanticAnalyzer::analyzeLifetimes(ast::ASTNode* node) {
     // Bu fonksiyon, D++'taki yaşam sürelerini analiz eder ve doğrular.
     // Referansların işaret ettiği verilerin ömründen daha uzun yaşamadığını garanti eder.
     // Bu genellikle ayrı bir veri akışı analizi veya constraint solving aşaması gerektiren karmaşık bir konudur.
     // AST üzerinde bir traversal yaparak referansların ve ait oldukları verilerin yaşam sürelerini belirlemeye çalışır.
     Reporter.reportNote(node->loc, "Yaşam süresi analizi (analyzeLifetimes) implement edilmedi."); // Geçici mesaj
}

void SemanticAnalyzer::resolveTraitsAndMethods(ast::CallExprAST* callExpr, Type* baseType) {
     // Bu fonksiyon, bir metot çağrısı (obj.method()) veya trait metodu çağrısı için uygun implementasyonu bulur.
     // obj'nin tipine (baseType) bakılır, ilgili impl blokları aranır ve metot çözümlenir.
     // Statik veya dinamik dispatch (trait object'ler için) burada belirlenir.
     Reporter.reportNote(callExpr->loc, "Trait ve Metot çözümlemesi (resolveTraitsAndMethods) implement edilmedi."); // Geçici mesaj
}

void SemanticAnalyzer::checkPatternValidity(ast::PatternAST* pattern, Type* targetType) {
     // analyzePattern içinde kısmen yapılıyor, ancak burada daha genel bir kontrol olabilir.
     // Örneğin, void tipe değişken bağlamak gibi anlamsız patternleri yakalar.
     Reporter.reportNote(pattern->loc, "Desen geçerliliği kontrolü (checkPatternValidity) implement edilmedi."); // Geçici mesaj
}

void SemanticAnalyzer::checkPatternExhaustiveness(ast::MatchExprAST* matchExpr, Type* matchedType) {
     // Bu fonksiyon, match ifadesindeki pattern kollarının eşleşilen tipin tüm olası değerlerini kapsayıp kapsamadığını kontrol eder.
     // Özellikle enum'lar ve boolean'lar gibi sonlu tipler için önemlidir.
     // '_' wildcard pattern'ı eksik durumları kapatmak için kullanılır.
     // Bu kontrol, derleyicinin en karmaşık kısımlarından biridir.
     Reporter.reportNote(matchExpr->loc, "Desen kapsayıcılığı kontrolü (checkPatternExhaustiveness) implement edilmedi."); // Geçici mesaj
}


} // namespace sema
} // namespace dppc
