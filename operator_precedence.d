module operator_precedence;

import syntax_kind; // Operatör token türleri için

// Operatör birleşme (associativity) türleri
enum Associativity {
    None,          // Birleşmez (örn: atama operatöründen önceki bazı operatörler)
    Left,          // Sol birleşmeli (örn: a + b + c -> (a + b) + c)
    Right          // Sağ birleşmeli (örn: a = b = c -> a = (b = c))
}

// Operatör öncelik seviyelerini belirten enum
// Daha yüksek sayı daha yüksek öncelik anlamına gelir.
// Değerler rastgeledir ve D++'ın gramerine göre belirlenmelidir.
enum Precedence {
    Lowest = 0,
    Assignment,      // =
    LogicalOr,       // ||
    LogicalAnd,      // &&
    Equality,        // ==, !=
    Comparison,      // <, <=, >, >=
    Additive,        // +, -
    Multiplicative,  // *, /
    Unary,           // +, -, !, & (referans al), * (dereference)
    CallAndMemberAccess, // ., (), []
    Highest          // Parantezler veya diğer en yüksek öncelikli yapılar
}

// Belirli bir operatör tokenının öncelik seviyesini döndüren fonksiyon
Precedence getOperatorPrecedence(SyntaxKind operatorKind) {
    final switch (operatorKind) {
        case SyntaxKind.EqualsToken: return Precedence.Assignment;
        case SyntaxKind.PipePipeToken: return Precedence.LogicalOr; // Eğer tanımlıysa
        case SyntaxKind.AmpersandAmpersandToken: return Precedence.LogicalAnd; // Eğer tanımlıysa
        case SyntaxKind.EqualsEqualsToken:
        case SyntaxKind.BangEqualsToken: return Precedence.Equality;
        case SyntaxKind.LessThanToken:
        case SyntaxKind.LessThanEqualsToken:
        case SyntaxKind.GreaterThanToken:
        case SyntaxKind.GreaterThanEqualsToken: return Precedence.Comparison;
        case SyntaxKind.PlusToken:
        case SyntaxKind.MinusToken: return Precedence.Additive;
        case SyntaxKind.StarToken:
        case SyntaxKind.SlashToken: return Precedence.Multiplicative;
        // Tekli operatörlerin önceliği genellikle yüksektir.
        case SyntaxKind.PlusToken: // Tekli +
        case SyntaxKind.MinusToken: // Tekli -
        case SyntaxKind.BangToken: // !
        case SyntaxKind.AmpersandToken: // & referans al
        case SyntaxKind.StarToken: // * dereference
             return Precedence.Unary;
        case SyntaxKind.OpenParenToken: // Fonksiyon çağrısı ()
        case SyntaxKind.OpenBracketToken: // Dizi/Index erişimi []
        case SyntaxKind.DotToken: // Üye erişimi .
             return Precedence.CallAndMemberAccess;

        // ... Diğer operatörler ve öncelikleri
        default: return Precedence.Lowest; // Varsayılan en düşük öncelik
    }
}

// Belirli bir operatör tokenının birleşme kuralını döndüren fonksiyon
Associativity getOperatorAssociativity(SyntaxKind operatorKind) {
    final switch (operatorKind) {
        case SyntaxKind.EqualsToken: return Associativity.Right; // Atama sağ birleşmelidir
        case SyntaxKind.PlusToken:
        case SyntaxKind.MinusToken:
        case SyntaxKind.StarToken:
        case SyntaxKind.SlashToken:
        case SyntaxKind.PipePipeToken:
        case SyntaxKind.AmpersandAmpersandToken:
        case SyntaxKind.EqualsEqualsToken:
        case SyntaxKind.BangEqualsToken:
        case SyntaxKind.LessThanToken:
        case SyntaxKind.LessThanEqualsToken:
        case SyntaxKind.GreaterThanToken:
        case SyntaxKind.GreaterThanEqualsToken:
            return Associativity.Left; // Çoğu operatör sol birleşmelidir

         case SyntaxKind.PlusToken: // Tekli +
         case SyntaxKind.MinusToken: // Tekli -
         case SyntaxKind.BangToken: // !
         case SyntaxKind.AmpersandToken: // & referans al
         case SyntaxKind.StarToken: // * dereference
             return Associativity.None; // Tekli operatörlerin birleşmesi yoktur (sağdan sola uygulanır)

         case SyntaxKind.DotToken: // Üye erişimi
             return Associativity.Left; // Genellikle sol birleşmelidir (obj.field1.field2)

        // ... Diğer operatörler ve birleşme kuralları
        default: return Associativity.None; // Varsayılan birleşmez
    }
}

// Örnek: Operatörlerin birleşme türü stringe çevirme
string to!string(Associativity assoc) {
    final switch (assoc) {
        case Associativity.None: return "None";
        case Associativity.Left: return "Left";
        case Associativity.Right: return "Right";
    }
}