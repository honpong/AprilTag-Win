#C++11

We use C++11, with the addition of `make_unique` from C++14.

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

<a name="commits"></a>
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

<a name="forking"></a>
#Forking and staying up to date

You can fork our repository by clicking on the "fork" button at the
top of the screen. This creates a complete clone of our repository
including all branches at:

```
https://github.intel.com/<your username>/tracker
```

Clone this fork to your local machine using:

```
git clone git@github.intel.com:<your username>/tracker
```

Your local clone now references to your private fork of the tracker
as `origin`. In order to make `master` on your machine track master on
the main slam repo instead of the copy of it on your fork, you
can add a new remote:

```
git remote add slam git@github.intel.com:slam/tracker
```

And then change the "upstream" for your local master branch to point
to this remote:

```
# update your local copy of the slam remote
git fetch slam
# set your local master branch to track slam/master instead of <your_username>/master
git branch master --set-upstream-to=slam/master
# update your local master with the slam master
git pull
```

Now your local master branch is set to track slam master. This
means you will be unable to push to it directly, so to start work you
should always checkout a new branch (which you can later use for a
pull request):

```
# create a new branch
git checkout -b <feature_name>
# push this branch to your fork and track it
git push -u origin <feature_name>
```

After you've done this, you can push from your `feature_name` branch
to your fork as normal with:

```
git push
```

#Pull requests

Contributions are handled via pull requests. Fork our repository
and create a feature branch as [described above](#forking).

You should conform to the commit style [described above](#commits).

Once your feature is tested, be sure you have rebased your feature
branch on the current version of master. Assuming your `master` branch
is setup to track `slam/master` as described above, do:

```
# update your local copy of master
git checkout master
git pull

# switch to your feature branch
git checkout <feature_name>
git rebase master

# fix whatever needs to be fixed
# test your feature again

# update your remote branch, force push is required because you have rebased
git push --force
```

When you are ready to submit your request, go to your feature branch
on the forked repository and click on the green "compare, review, and
create pull request" button. This will provide a new page for comments
on the request. You can find an example pull request
[here](https://github.intel.com/slam/tracker/pull/135)
