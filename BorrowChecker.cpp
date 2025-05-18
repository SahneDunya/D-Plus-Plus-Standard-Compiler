#include "BorrowChecker.h"
#include <iostream> // Debug için

// AST düğümlerinin spesifik tiplerini kullanmak için include'lar
#include "../parser/AST.h"

namespace dppc {
namespace sema {

// BorrowChecker Constructor
BorrowChecker::BorrowChecker(ErrorReporter& reporter, SymbolTable& symbols, TypeSystem& types)
    : Reporter(reporter), Symbols(symbols), Types(types)
{
    // Başlangıç ayarları veya durum başlatma (gerekiyorsa)
}

// --- Semantik Analiz Sırasında Çağrılan Kontrol Fonksiyonları ---

void BorrowChecker::onVariableDeclaration(ast::LetStmtAST* letStmt, Symbol* symbol) {
    // Yeni tanımlanan değişkenin başlangıç durumunu ayarla.
     let x = value; veya let mut x = value;
    // Değişken varsayılan olarak Owned durumdadır (eğer tipi Copy değilse ve initializer'dan move ediliyorsa).
    // Eğer initializer yoksa (let x: T;), Uninitialized durumdadır.
    // Eğer initializer varsa ve tipi Copy ise, Copy işlemi olur, initializer durumu değişmez.
    // Eğer initializer varsa ve tipi Copy değilse, Move işlemi olur, initializer sembolü Moved durumuna geçer.

    if (letStmt->Initializer) {
        // Initializer ifadesinin tipini ve Sembolünü al
        // Bu bilgi SemanticAnalyzer tarafından analyzeExpr sırasında AST düğümüne eklenmiş olmalı.
         Type* initializerType = letStmt->Initializer->resolvedType; // Varsayılan AST anotasyonu
         Symbol* initializerSymbol = nullptr; // Eğer initializer bir değişkense
         if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(letStmt->Initializer.get())) {
            initializerSymbol = idExpr->resolvedSymbol;
         }

        Type* initializerType = nullptr; // Sema'dan gelen tipi burada almanız gerekir
         checkMoveOrCopy(letStmt->Initializer.get(), initializerType, letStmt->loc); // Başlangıç değeri için move/copy kontrolü

        // Yeni değişkenin durumunu ayarla
        if (symbol) {
            symbol->state = OwnershipState::Owned; // Başlangıçta sahiplik
            symbol->borrowCount = 0;
             symbol->isMutable zaten LetStmtAST ve SymbolTable'da ayarlanmıştır.
        }
    } else {
         // Initializer yoksa Uninitialized
         if (symbol) {
             symbol->state = OwnershipState::Uninitialized;
             symbol->borrowCount = 0;
         }
    }

     Reporter.reportNote(letStmt->loc, "onVariableDeclaration kontrolü çalıştı (Placeholder)."); // Debug
}

void BorrowChecker::onAssignment(ast::BinaryOpExprAST* assignExpr, Type* targetType, Type* valueType) {
    // Atama işlemi: target = value
    // Target'ın mutable lvalue olduğunu kontrol et (Sema tarafından yapılmalı ama BC de kontrol edebilir)
     checkMutability(assignExpr->Left.get(), true); // Target sol taraf ifadesi

    // Value ifadesi için move/copy kontrolü
     checkMoveOrCopy(assignExpr->Right.get(), valueType, assignExpr->loc);

    // Target (sol taraf) bir değişken ise, o değişkenin durumunu güncelle (Owned olur)
    // Eğer target bir alan erişimi veya dizi elemanı ise, ana nesnenin veya dizinin durumu etkilenebilir.
     if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(assignExpr->Left.get())) {
        if (Symbol* targetSymbol = idExpr->resolvedSymbol) {
    //        // Atama başarılı olursa, target sembolü tekrar Owned durumuna geçer (eğer daha önce moved veya dropped ise)
    //        // Bu durum karmaşıktır, çünkü target daha önce borrowed olabilir. Tüm borrow'ları geçersiz kılmak gerekebilir.
             targetSymbol->state = OwnershipState::Owned; // Bu çok basitleştirilmiş
             targetSymbol->borrowCount = 0;
    //        // TODO: Atama kurallarını tam implemente et: borrow varken atama geçerli mi?
        }
     }

     Reporter.reportNote(assignExpr->loc, "onAssignment kontrolü çalıştı (Placeholder)."); // Debug
}

void BorrowChecker::onVariableUse(ast::IdentifierExprAST* idExpr, Symbol* symbol) {
    // Bir değişken (identifier) kullanıldığında (değeri okunduğunda) kuralları kontrol et.
    // Değişken moved veya dropped durumda olmamalıdır.
    // Eğer mutable borrow durumundaysa, direct kullanımı genellikle hatadır.
    // Eğer shared borrow durumundaysa, direct kullanımı genellikle geçerlidir (mutasyona neden olmuyorsa).
    if (symbol) {
        if (symbol->state == OwnershipState::Moved) {
            Reporter.reportError(idExpr->loc, "Taşınmış (moved) değer kullanılıyor: '" + symbol->Name + "'");
            Reporter.reportNote(symbol->loc, "'" + symbol->Name + "' burada taşındı/move edildi."); // Move'un gerçekleştiği yeri takip etmek gerekir
        } else if (symbol->state == OwnershipState::Dropped) {
             Reporter.reportError(idExpr->loc, "Kapsam dışına çıkmış (dropped) değer kullanılıyor: '" + symbol->Name + "'");
             Reporter.reportNote(symbol->loc, "'" + symbol->Name + "' bu kapsamdan çıkıldığında düşürüldü (dropped)."); // Dropped'ın gerçekleştiği yeri takip etmek gerekir
        } else if (symbol->state == OwnershipState::BorrowedMutable) {
            // Mutable borrow varken direct kullanım genellikle hatadır.
             Reporter.reportError(idExpr->loc, "Değer, aktif bir mutable borrow varken kullanılıyor: '" + symbol->Name + "'");
             // TODO: Mutable borrow'un nerede başladığını raporla.
        }
        // Shared borrow veya Owned durumları genellikle direct kullanım için geçerlidir.
    }
     Reporter.reportNote(idExpr->loc, "onVariableUse kontrolü çalıştı (Placeholder)."); // Debug
}

void BorrowChecker::onBorrowExpression(ast::BorrowExprAST* borrowExpr, Type* operandType) {
    // & veya &mut ifadesi analiz edildiğinde kuralları kontrol et.
    // Operandın (ödünç alınan değerin) lvalue olduğundan emin ol (Sema tarafından yapılmalı ama burada da kontrol edilebilir).
    // Operandın bir sembol tarafından temsil edildiğini varsayalım (değişken, alan, dizi elemanı).
     if (!borrowExpr->Operand->isLValue) { ... hata ... } // AST'de isLValue bayrağı olduğunu varsayalım

    // Operand sembolünü bul
    Symbol* operandSymbol = nullptr;
      if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(borrowExpr->Operand.get())) {
         operandSymbol = idExpr->resolvedSymbol;
      }
     // TODO: Operand karmaşık bir ifade olabilir (alan erişimi, dizi elemanı), bunların temsil ettiği "yerin" (location) sembolünü veya durumunu takip etmek gerekir.


    if (operandSymbol) {
        if (borrowExpr->IsMutable) { // &mut borrow
            // Operand mutable olmalı
            if (!operandSymbol->isMutable) {
                 Reporter.reportError(borrowExpr->loc, "Immutable değerden mutable borrow (&mut) yapılamaz: '" + operandSymbol->Name + "'");
            }
            // Operand şu anda başka bir borrow tarafından kullanılmıyor olmalı (shared veya mutable)
            if (operandSymbol->state == OwnershipState::BorrowedShared || operandSymbol->state == OwnershipState::BorrowedMutable) {
                 Reporter.reportError(borrowExpr->loc, "Değer, aktif bir borrow varken mutable borrow (&mut) yapılamaz: '" + operandSymbol->Name + "'");
                 // TODO: Çakışan borrow'un nerede başladığını raporla.
            }
            // Durumu güncelle
            operandSymbol->state = OwnershipState::BorrowedMutable;
            operandSymbol->borrowCount = 0; // Mutable borrow varken shared count sıfırlanır
            // TODO: Mutable borrow'un lifetime'ını başlat.
        } else { // & borrow
            // Operand şu anda mutable borrow tarafından kullanılmıyor olmalı
            if (operandSymbol->state == OwnershipState::BorrowedMutable) {
                Reporter.reportError(borrowExpr->loc, "Değer, aktif bir mutable borrow varken shared borrow (&) yapılamaz: '" + operandSymbol->Name + "'");
                // TODO: Mutable borrow'un nerede başladığını raporla.
            }
            // Durumu güncelle (eğer zaten borrowedshared değilse)
            if (operandSymbol->state != OwnershipState::BorrowedShared) {
                 operandSymbol->state = OwnershipState::BorrowedShared;
            }
            operandSymbol->borrowCount++; // Shared borrow sayısını artır
            // Eğer operand immutable ise, durumu ImmutableBorrowed olarak da işaretleyebiliriz.
            if (!operandSymbol->isMutable) {
                 operandSymbol->state = OwnershipState::ImmutableBorrowed;
            }
            // TODO: Shared borrow'un lifetime'ını başlat.
        }

        // TODO: Borrow edilen referansın tipine (ReferenceType) yaşam süresi bilgisini ekle.
         borrowExpr->resolvedType = Types.getReferenceType(operandType, borrowExpr->IsMutable, createdLifetime); // updated type with lifetime
    } else {
         // Operand bir sembol tarafından temsil edilmiyorsa (örn: literal), ondan borrow yapılamaz.
         // Ancak literals/const values static lifetime'a sahip olabilir. Bu daha karmaşık.
         // Şimdilik sadece semboller üzerinden borrow kontrolü yapalım.
         Reporter.reportError(borrowExpr->loc, "Sadece değişkenler, alanlar veya dizi elemanlarından ödünç alma (& veya &mut) yapılabilir.");
    }

     Reporter.reportNote(borrowExpr->loc, "onBorrowExpression kontrolü çalıştı (Placeholder)."); // Debug
}

void BorrowChecker::onDereferenceExpression(ast::DereferenceExprAST* derefExpr, Type* operandType) {
     // * ifadesi analiz edildiğinde kuralları kontrol et.
     // Operandın (dereference edilenin) pointer veya referans olduğundan emin ol (Sema zaten kontrol eder).
     // Eğer operand bir referans ise, bu referansın yaşam süresinin hala geçerli olduğunu kontrol et.
     // Eğer referans bir mutable borrow'dan geliyorsa, dereference sonucu mutable lvalue'dur ve mutasyona izin verir.
     // Eğer referans bir shared borrow'dan geliyorsa, dereference sonucu immutable lvalue'dur ve mutasyona izin vermez.
     // Eğer referans moved veya dropped bir değişkenden geliyorsa, bu bir use-after-free hatasıdır.

     // Operand TypeSystem'daki bir ReferenceType veya PointerType olmalı
      if (auto refType = dynamic_cast<ReferenceType*>(operandType)) {
     //    // Check lifetime validity of the reference represented by the operand expression
          analyzeLifetimes aşamasında atanan lifetime bilgisini burada kullanın.
          checkLifetimeValidity(derefExpr->Operand.get(), operandType);
     //
     //    // Dereference sonucu mutable ise (referans mutable ise)
          derefExpr->isLValue = true;
          derefExpr->isMutable = refType->isMutable();
      } else if (auto ptrType = dynamic_cast<PointerType*>(operandType)) {
     //     // Pointer dereference (D'deki gibi) genellikle null pointer kontrolü gerektirir (runtime veya static analiz)
     //     // Ve memory safety D++ sahiplik kuralları ile nasıl entegre olacak?
            checkDereferenceRules(derefExpr, operandType); // Placeholder
            derefExpr->isLValue = true; // Pointer dereference sonucu atanabilir
      }

     Reporter.reportNote(derefExpr->loc, "onDereferenceExpression kontrolü çalıştı (Placeholder)."); // Debug
}


void BorrowChecker::onFunctionCall(ast::CallExprAST* callExpr, FunctionType* calleeType) {
    // Fonksiyon çağrısı argümanları için sahiplik/borrowing kurallarını kontrol et.
    // Fonksiyon imzasındaki her parametrenin nasıl değer aldığını (by value, by shared ref, by mutable ref) bilmek gerekir.
    // Bu bilgi FunctionType nesnesinde veya ParameterAST düğümlerinde saklanmalıdır.
    // Örneğin, eğer parametre ByValue/Move alıyorsa, karşılık gelen argüman move edilebilir olmalı ve move edildikten sonra argümanın kaynak sembolü Moved durumuna geçmelidir.
    // Eğer parametre BySharedRef alıyorsa, karşılık gelen argüman shared borrow edilebilir olmalı ve borrow sırasında Operand Symbol state'i ve count'u güncellenmelidir.
    // Eğer parametre ByMutableRef alıyorsa, karşılık gelen argüman mutable borrow edilebilir olmalı ve borrow sırasında Operand Symbol state'i ve count'u güncellenmelidir.

    // FunctionType'dan veya Symbol'den parametre geçirme mekanizmasını alın
     std::vector<ParameterPassingStyle> paramPassingStyles = calleeSymbol->paramPassingStyles; // Örnek

    // Argümanlar üzerinde döngü yap ve kontrol et
     for (size_t i = 0; i < callExpr->Args.size(); ++i) {
         ast::ExprAST* argExpr = callExpr->Args[i].get();
         Type* argType = argExpr->resolvedType; // Sema tarafından belirlenen argüman tipi
         ParameterPassingStyle passingStyle = paramPassingStyles[i];
    //
         switch (passingStyle) {
             case ParameterPassingStyle::ByValue_Move:
                  checkMoveOrCopy(argExpr, argType, argExpr->loc);
    //             // Eğer argExpr bir sembolse, onu Moved olarak işaretle.
                  if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(argExpr)) { idExpr->resolvedSymbol->state = OwnershipState::Moved; }
                 break;
             case ParameterPassingStyle::BySharedReference:
                  checkSharedBorrow(argExpr, argType, someLifetime);
    //             // Eğer argExpr bir sembolse, onu BorrowedShared olarak işaretle ve count'u artır.
                 break;
             case ParameterPassingStyle::ByMutableReference:
                  checkMutableBorrow(argExpr, argType, someLifetime);
    //             // Eğer argExpr bir sembolse, onu BorrowedMutable olarak işaretle.
                 break;
    //         // TODO: Diğer geçirme stilleri (ByCopy, ByImmutableReference, ref, inout vb.)
         }
     }
     Reporter.reportNote(callExpr->loc, "onFunctionCall kontrolü çalıştı (Placeholder)."); // Debug
}

void BorrowChecker::onReturnStatement(ast::ReturnStmtAST* returnStmt, Type* returnedType, Type* functionReturnType) {
    // Dönüş değeri ifadesi varsa, bu değerin fonksiyon dışına move edilebildiğini kontrol et.
    if (returnStmt->ReturnValue) {
         checkMoveOrCopy(returnStmt->ReturnValue.get(), returnedType, returnStmt->loc);
        // Dönüş değeri bir sembolse, fonksiyon kapsamı dışına çıktığında durumu Dropped yerine Moved olabilir.
        // Bu, çağıranın sahipliği alacağını belirtir.
         if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(returnStmt->ReturnValue.get())) { idExpr->resolvedSymbol->state = OwnershipState::Moved; } // Kapsam dışına taşındı
    }
     Reporter.reportNote(returnStmt->loc, "onReturnStatement kontrolü çalıştı (Placeholder)."); // Debug
}

void BorrowChecker::onScopeExit(SymbolTable::Scope* scope) {
    // Bir kapsamdan çıkıldığında, bu kapsamda tanımlanan tüm sembollerin (Owned durumda olanlar) düşürülmesi (drop) gerektiğini kontrol et.
    // Ayrıca, bu kapsam içinde başlayan borrow'ların sonlandığını ve ilgili sembollerin borrowCount'larının azaldığını kontrol et.
    // Kapsam içinde başlayan borrow'lar, kapsam dışındaki verilere işaret ediyorsa, bu borrow'lar geçerli olmaya devam edebilir
    // (örneğin, fonksiyon parametreleri olarak alınan referanslar).
    // Ancak kapsam içinde Owned olan bir değere yapılan borrow'lar, kapsamdan çıkıldığında geçersiz hale gelir.

    // Scope içindeki semboller üzerinde döngü yap
     for (auto const& [name, symbol] : scope->Symbols) {
        if (symbol.Kind == SymbolKind::Variable) { // Sadece değişkenler için sahiplik/borrowing durumu takip edilir
            if (symbol.state == OwnershipState::Owned || symbol.state == OwnershipState::Uninitialized) {
    //            // Owned veya Uninitialized değişkenler kapsam dışına çıkınca düşürülür.
                 symbol.state = OwnershipState::Dropped;
    //            // TODO: Tipin destructor'ı varsa çağrılmasını işaretle.
            } else if (symbol.state == OwnershipState::BorrowedShared ||
                       symbol.state == OwnershipState::BorrowedMutable ||
                       symbol.state == OwnershipState::ImmutableBorrowed)
             {
    //             // Bu borrow kapsam içinde başladıysa ve scope dışına taşınmıyorsa, bu noktada borrow sona erer.
                  checkLifetimeValidity(symbol.borrowedFromExpr, symbol.borrowedFromType); // Borrow'un geçerliliğini kontrol et
    //             // İlgili kaynak sembolün borrow count'unu azalt veya state'ini geri döndür.
    //             // Bu karmaşık bir geri referanslama gerektirir.
             }
    //        // Moved veya Dropped durumundaki semboller zaten işlenmiştir.
        }
    //    // TODO: Diğer sembol türlerinin (Fonksiyon, Tip vb.) kapsam dışına çıkışı için gerekli işlemler (SymbolTable yapar)
     }

    Reporter.reportNote(scope->loc, "onScopeExit kontrolü çalıştı (Placeholder)."); // Debug
}


// --- Yardımcı Fonksiyonlar Implementasyonları (Basit Placeholder'lar) ---

bool BorrowChecker::checkSymbolUsability(Symbol* symbol, const SourceLocation& loc, const std::string& usageDescription) {
    if (!symbol) return false; // Geçersiz sembol

    if (symbol->state == OwnershipState::Moved) {
        Reporter.reportError(loc, "Taşınmış (moved) değer kullanılıyor (" + usageDescription + "): '" + symbol->Name + "'");
        // TODO: Move'un gerçekleştiği yeri not olarak ekle.
        return false;
    }
     if (symbol->state == OwnershipState::Dropped) {
         Reporter.reportError(loc, "Kapsam dışına çıkmış (dropped) değer kullanılıyor (" + usageDescription + "): '" + symbol->Name + "'");
          // TODO: Dropped'ın gerçekleştiği yeri not olarak ekle.
         return false;
     }
    if (symbol->state == OwnershipState::BorrowedMutable) {
        // Mutable borrow varken kullanımı kontrol et. Sadece mutable borrow'u yapan referans geçerlidir.
        // Başka bir kullanım (read, write, shared borrow, mutable borrow) hatadır.
        // Bu noktadaki kullanımın mutable borrow yapan referans aracılığıyla olup olmadığını bilmek gerekir.
        // Basitlik için, mutable borrow varken direct kullanımı hata varsayalım.
         Reporter.reportError(loc, "Değer, aktif bir mutable borrow varken kullanılıyor (" + usageDescription + "): '" + symbol->Name + "'");
         // TODO: Çakışan borrow'un nerede başladığını raporla.
         return false;
    }
    // TODO: Daha fazla durum kontrolü (ImmutableBorrowed iken mutable işlem yapma gibi).

    return true; // Şimdilik kullanılabilir kabul et
}

bool BorrowChecker::checkMoveOrCopy(ast::ExprAST* sourceExpr, Type* sourceType, const SourceLocation& usageLoc) {
    if (!isValidType(sourceType)) return false; // Geçersiz kaynak tip

    // Kaynak ifadenin bir lvalue (değişken, alan, dizi elemanı) olup olmadığını belirle.
     if (!sourceExpr->isLValue) { ... bu bir değer, kopyalanabilir/move edilebilir mi bak ... } // AST'de isLValue olduğunu varsayalım

    // Kaynak bir sembol tarafından temsil ediliyorsa (değişken)
     Symbol* sourceSymbol = nullptr;
      if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(sourceExpr)) {
          sourceSymbol = idExpr->resolvedSymbol;
      }
     // TODO: Alan erişimi veya dizi elemanı gibi karmaşık lvalue'lar için sembol/durum takibi

    if (sourceSymbol) {
        // Sembol kullanılabilir mi kontrol et (moved, dropped, borrowed?)
        if (!checkSymbolUsability(sourceSymbol, usageLoc, "move veya copy")) {
             return false; // Kullanılamaz
        }

        // Tip kopyalanabilir (Copy) değilse, bu bir move işlemidir.
        if (!sourceType->isCopyType()) {
            // Eğer sembol Owned durumdaysa, artık Moved durumuna geçer.
            if (sourceSymbol->state == OwnershipState::Owned) {
                 sourceSymbol->state = OwnershipState::Moved;
                 // TODO: Move'un gerçekleştiği yeri (usageLoc) sembolde veya ayrı bir veri yapısında kaydet.
                 Reporter.reportNote(usageLoc, "Değer burada taşındı (moved)."); // Debug amaçlı
            } else {
                // Owned olmayan bir değer move edilemez (zaten borrowed veya moved olabilir).
                 Reporter.reportError(usageLoc, "Owned olmayan değer taşınmaya çalışılıyor: '" + sourceSymbol->Name + "'");
                 return false;
            }
        } else {
            // Tip kopyalanabilir (Copy) ise, bu bir copy işlemidir. Kaynak sembolün durumu değişmez.
             Reporter.reportNote(usageLoc, "Değer burada kopyalandı (copy)."); // Debug amaçlı
        }
         return true; // İşlem geçerli
    } else {
         // Kaynak bir lvalue değilse (örn: literal, fonksiyon çağrısı sonucu rvalue), her zaman kopyalanabilir/move edilebilir
         // (genellikle RValue'lar için move semantics yoktur, kopyalanırlar veya geçici nesnelerdir).
         // Eğer tipi Copy değilse, bu bir move işlemi olarak değerlendirilmez, değer geçerli yaşam süresi içinde kullanılır.
         // Bu durum daha basittir, genelde hata vermez.
         Reporter.reportNote(usageLoc, "RValue ifadesi için move/copy kontrolü çalıştı (Placeholder)."); // Debug
         return true;
    }
}

// TODO: checkSharedBorrow, checkMutableBorrow, checkDereference, checkAssignment, checkFunctionCallArguments, checkReturnStatement
// Bu fonksiyonlar da yukarıdaki checkMoveOrCopy ve checkSymbolUsability gibi yardımcıları kullanarak implement edilmelidir.
// Lifetime analizi bu fonksiyonların içine veya ayrı bir analiz aşamasına entegre edilmelidir.


} // namespace sema
} // namespace dppc
