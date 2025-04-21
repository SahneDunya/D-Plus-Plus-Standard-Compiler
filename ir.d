module ir;

import data_types; // Eğer spesifik D++ veri tiplerini kullanacaksanız import edin

// Çok temel bir Ara Temsil (IR) komut türü enum'ı
// Gerçek bir IR çok daha fazla komut türü içerecektir.
enum IrOpCode {
    Nop,         // No operation
    LoadConstant, // Sabit bir değeri yükle (Register, Value)
    LoadVariable, // Bir değişkenin değerini yükle (Register, VariableInfo) - Varsayım: Semantik analizden gelen değişken bilgisi
    StoreVariable, // Bir değişkene değer ata (VariableInfo, Register)
    Add,         // Toplama (DestRegister, SourceRegister1, SourceRegister2)
    Subtract,    // Çıkarma (DestRegister, SourceRegister1, SourceRegister2)
    Multiply,    // Çarpma
    Divide,      // Bölme
    Call,        // Fonksiyon çağrısı (DestRegister/null, FunctionInfo, Args...) - Varsayım: Semantik analizden gelen fonksiyon bilgisi
    Return,      // Fonksiyondan dön (Register/null)
    Label,       // Hedef etiket (LabelID)
    Jump,        // Koşulsuz atlama (LabelID)
    BranchIfTrue, // Koşullu atlama (ConditionRegister, LabelID)
    // ... Diğer komutlar (karşılaştırmalar, bellek erişimi, tip dönüşümleri vb.)
}

// IR komutlarını işleyecek sanal kayıtlar (virtual registers)
// Gerçek backend bunları fiziksel kayıtlarla veya stack slotları ile eşleyecektir.
alias Register = int; // Basitçe bir int ile temsil edelim

// Sabit değerler için bir yapı
struct ConstantValue {
     data_types.DppType type; // Sabitin türü (int, float, string vb.)
     union { ... } value; // Sabit değeri tutacak union
    long intValue; // Şimdilik sadece int varsayalım
    string stringValue; // Şimdilik sadece string varsayalım
     bool boolValue;
}

// IR Komutunu temsil eden yapı
struct IrInstruction {
    IrOpCode opCode;
    Register dest; // Hedef kayıt (bazı komutlar için)
    Register src1; // Kaynak kayıt 1 (bazı komutlar için)
    Register src2; // Kaynak kayıt 2 (bazı komutlar için)
    ConstantValue constant; // Sabit değer (LoadConstant için)
     VariableInfo variable; // Değişken bilgisi (LoadVariable, StoreVariable için) - Semantik analizden gelir
     FunctionInfo function; // Fonksiyon bilgisi (Call için) - Semantik analizden gelir
    int labelId; // Etiket ID'si (Label, Jump, BranchIfTrue için)
    // Diğer komutlara özel alanlar
}

// Bir fonksiyonun veya kod bloğunun IR temsilini tutacak yapı
struct IrBlock {
    int id; // Bloğun benzersiz ID'si
    IrInstruction[] instructions; // Bu bloktaki komutlar
    // Kontrol akışı bilgisi (hangi bloklara atlayabilir vb.)
}

// Bir fonksiyonun veya modülün tam IR temsilini tutacak yapı
struct IrFunction {
     FunctionInfo info; // Fonksiyonun bilgisi - Semantik analizden gelir
    IrBlock[] blocks; // Fonksiyonun kod blokları
    // Yerel değişkenler, parametreler vb. hakkında bilgi
}

// Tüm programın IR temsilini tutacak yapı
struct IrProgram {
    IrFunction[] functions; // Programdaki fonksiyonlar
    // Global değişkenler, veri bölümleri vb. hakkında bilgi
}

// Semantik analiz sonucunda oluşturulan AST'yi IR'a çevirecek fonksiyon (Frontend veya ayrı bir IR katmanında olabilir)
// Şimdilik burada bir imzasını belirtelim.
 IrProgram generateIR(ASTNode* syntaxTree);