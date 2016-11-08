#Coding style

As this repository has grown, there have been many different styles
used in it. For exisiting code, the first and most important guidline
is:

- As much as possible, keep the style of the part of the code you are
  working in

For new code, try to maintain the following:

- Class names / struct names are lowercase with underscores (example
  `sensor_fusion`)
- Function names are lowercase with underscores. When they are global,
  they start with the name of the file they are in
  (`filter_image_measurement`). If they take an object-like struct to
  manipulate (as happens often in `filter.cpp`) it is the first
  parameter of the function.
- Opening brace on the same line as the enclosing statement (rather
  than on the next line).
- Indents are four spaces. No tabs.
- Avoid stray whitespace and windows newline characters on the end of
  each line
- Exceptionally, in `rc_tracker.{cpp/h}` we've adopted struct names
  are `rc_CapitalizedName`, function names are `rc_camelCase`
- Avoid using new/delete or malloc, favor shared/unique pointers or
  stack allocation.
- Variable names should be informative whenever possible. Math
  variables can be short if the meaning is clear (e.g. `w` for angular
  velocity).

#Commits

- Try to make your git commits as small and self contained as possible.
The commits should not contain extra changes that are unrelated or
purely formatting.

- Follow the following format for a commit message:

```
<main_area>: short message, <70 characters with main idea

After the line break, you can add a longer commit message (again, wrap
at 70 characters). This can explain the reasoning behind the change
and the components affected. If you are closing a github issue, you
can write closes #<issue_number> to automatically close it.
```

#Pull requests

Contributions are handled via pull requests. Fork our repository
(using the github fork button) and make your contributions using the
commit style above. When you are ready to submit your request, go to
your forked repository and click on the green "compare, review, and
create pull request" button. This will provide a new page for comments
on the request. You can find an example pull request
[here](https://github.intel.com/sensorfusion/sensorfusion/pull/135)

