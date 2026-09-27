
// WorkshopC off // good: reason on the next line
// Reason: legacy enum that cannot be changed
enum good_line_comment_reason
{
    good_line_comment_reason_value
};
// WorkshopC on

// WorkshopC off // good: block comment reason on the next line
/* Reason: legacy enum that cannot be changed */
enum good_block_comment_reason
{
    good_block_comment_reason_value
};
// WorkshopC on

// WorkshopC off // good: indented reason on the next line
    //   Reason: extra whitespace around the comment is fine
enum good_indented_reason
{
    good_indented_reason_value
};
// WorkshopC on

// WorkshopC off // bad: no comment on the next line
enum bad_no_reason
{
    bad_no_reason_value
};
// WorkshopC on

// WorkshopC off // bad: next line is a comment that does not start with 'Reason: '
// because I said so
enum bad_wrong_comment
{
    bad_wrong_comment_value
};
// WorkshopC on

// WorkshopC off // bad: 'Reason: ' is case sensitive
// reason: legacy enum that cannot be changed
enum bad_lowercase_reason
{
    bad_lowercase_reason_value
};
// WorkshopC on

// WorkshopC off // bad: the reason must be on the very next line
 
// Reason: legacy enum that cannot be changed
enum bad_reason_after_blank_line
{
    bad_reason_after_blank_line_value
};
// WorkshopC on

// WorkshopC off // bad: reason text is empty
// Reason: 
enum bad_empty_reason
{
    bad_empty_reason_value
};
// WorkshopC on

enum this_enum_is_not_ok // bad: not suppressed
{
    rule_applied
};
