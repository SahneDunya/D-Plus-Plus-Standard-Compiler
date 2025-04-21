module syntax_kind;

import std.string; // to!string için gerekebilir

// D++ dilindeki tüm token ve AST düğüm türlerini tanımlayan enum
enum SyntaxKind {
    // Özel Token Türleri
    UnknownToken,        // Tanımlanamayan token (hata durumları için)
    EndOfFileToken,      // Dosya sonu

    // Tek Karakterlik Tokenlar
    PlusToken,           // +
    MinusToken,          // -
    StarToken,           // *
    SlashToken,          // /
    EqualsToken,         // =
    OpenParenToken,      // (
    CloseParenToken,     // )
    OpenBraceToken,      // {
    CloseBraceToken,     // }
    OpenBracketToken,    // [
    CloseBracketToken,   // ]
    SemicolonToken,      // ;
    CommaToken,          // ,
    DotToken,            // .
    ColonToken,          // :
    // ... Diğer tek karakterlik tokenlar

    // İki veya Daha Fazla Karakterlik Tokenlar
    EqualsEqualsToken,   // ==
    BangEqualsToken,     // !=
    LessThanToken,       // <
    LessThanEqualsToken, // <=
    GreaterThanToken,    // >
    GreaterThanEqualsToken, // >=
    ArrowToken,          // -> (Match ifadesi veya lambda için olabilir)
    // ... Diğer çok karakterli tokenlar

    // Literaller (Sabit Değerler)
    IdentifierToken,      // Değişken, fonksiyon vb. isimleri
    IntegerLiteralToken,  // 123, 456 gibi tam sayılar
    FloatingPointLiteralToken, // 1.23, 4.5e-2 gibi ondalıklı sayılar
    StringLiteralToken,   // "Merhaba" gibi metinler
    BooleanLiteralToken,  // true, false
    // ... Diğer literal türleri (CharacterLiteralToken vb.)

    // Anahtar Kelimeler (Keywords)
    IfKeyword,            // if
    ElseKeyword,          // else
    SwitchKeyword,        // switch
    MatchKeyword,         // match (D++'a özel)
    FnKeyword,            // fn (D++'a özel - fonksiyon tanımı)
    LetKeyword,           // let (D++'a özel - değişken bildirimi)
    MutKeyword,           // mut (D++'a özel - değişkenin değiştirilebilirliği)
    StructKeyword,        // struct
    ClassKeyword,         // class
    EnumKeyword,          // enum
    ImportKeyword,        // import
    ReturnKeyword,        // return
    // ... Diğer D++ anahtar kelimeleri

    // AST Düğüm Türleri (Parser Tarafından Oluşturulur)
    ProgramNode,                 // Tüm programı temsil eden kök düğüm
    FunctionDeclarationNode,     // Fonksiyon bildirimi
    VariableDeclarationNode,     // Değişken bildirimi (let mut x = 5;)
    IfStatementNode,             // If-else ifadesi
    SwitchStatementNode,         // Switch ifadesi
    MatchExpressionNode,         // Match ifadesi/desen eşleştirme (Rust'tan esinlenilmiş)
    BlockStatementNode,          // Kod bloğu ({ ... })
    ExpressionStatementNode,     // Bir ifadeyi içeren statement (x = 5;)
    ReturnStatementNode,         // Return ifadesi

    // İfade Düğümleri
    BinaryOperatorExpressionNode, // İkili operatör içeren ifadeler (a + b)
    UnaryOperatorExpressionNode,  // Tekli operatör içeren ifadeler (-a, !b)
    LiteralExpressionNode,        // Literal değerler (123, "abc", true)
    IdentifierExpressionNode,     // Tanımlayıcı kullanımı (değişken veya fonksiyon adı)
    CallExpressionNode,           // Fonksiyon çağrısı (myFunc(arg1, arg2))
    MemberAccessExpressionNode,   // Üye erişimi (obj.field)
    IndexAccessExpressionNode,    // Dizi/koleksiyon elemanına erişim (arr[i])
    // ... Diğer ifade türleri

    // Desen Eşleştirme Düğümleri (Match için)
    LiteralPatternNode,          // Match deseninde literal (1, "abc")
    IdentifierPatternNode,       // Match deseninde değişken yakalama (x)
    StructPatternNode,           // Match deseninde struct eşleştirme
    // ... Diğer desen türleri

    // Diğer Yapı Düğümleri
    ParameterNode,               // Fonksiyon parametresi
    TypeNode,                    // Tip belirtimi (int, string, MyStruct)
    ImportDeclarationNode,       // Import bildirimi
    // ... Dilin gramerindeki diğer yapılar için düğümler
}

// SyntaxKind enum değerini stringe çevirmek için yardımcı fonksiyon (debugging için kullanışlı)
string to!string(SyntaxKind kind) {
    final switch (kind) {
        // Tokenlar
        case SyntaxKind.UnknownToken: return "UnknownToken";
        case SyntaxKind.EndOfFileToken: return "EndOfFileToken";
        case SyntaxKind.PlusToken: return "PlusToken";
        case SyntaxKind.MinusToken: return "MinusToken";
        // ... Diğer tokenlar

        // Literaller
        case SyntaxKind.IdentifierToken: return "IdentifierToken";
        case SyntaxKind.IntegerLiteralToken: return "IntegerLiteralToken";
        // ... Diğer literaller

        // Anahtar Kelimeler
        case SyntaxKind.IfKeyword: return "IfKeyword";
        case SyntaxKind.MatchKeyword: return "MatchKeyword";
        case SyntaxKind.FnKeyword: return "FnKeyword";
        // ... Diğer anahtar kelimeler

        // AST Düğümleri
        case SyntaxKind.ProgramNode: return "ProgramNode";
        case SyntaxKind.FunctionDeclarationNode: return "FunctionDeclarationNode";
        case SyntaxKind.MatchExpressionNode: return "MatchExpressionNode";
        // ... Diğer AST düğümleri

        // Default durum: Eğer yukarıdaki eşleşmezse enum'ın kendisini stringe çevir
        default: return format("SyntaxKind(%s)", kind);
    }
}