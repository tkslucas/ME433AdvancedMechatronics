In this exercise you will practice using git on a repo along with the whole class. You will need a recipe, something you like to cook and want to share with the world.

The results of the recipe book are pushed to this website: [https://ndm736.github.io/ME433.Kitchen/](https://ndm736.github.io/ME433.Kitchen/). The repo you will edit uses [Github Pages](https://pages.github.com/) and [Jekyll](https://jekyllrb.com/) to publish the website. Instead of HTML, the content is written in wiki style markdown language. I've picked the [Dinky Theme](https://pages-themes.github.io/dinky/) for the default formatting.

The repo with the content is [https://github.com/ndm736/ME433.Kitchen](https://github.com/ndm736/ME433.Kitchen). I will add you as a collaborator so that you can push your changes without making a pull request (be sure to send me your git name in the Initial Git Check assignment on Canvas first).

- Clone the repo using Github Desktop (or the CLI if you are using that), only after you get the notification email that I added you as a collaborator.
- Using my example as a guide, add a new file to the recipes folder, with a .md extension.
- Edit the file with a text editor, enter your recipe. [This](https://raw.githubusercontent.com/pages-themes/dinky/master/index.md) raw markdown looks like [this](https://pages-themes.github.io/dinky/) when published.
- Edit index.md to add a link to your recipe.
- Commit and push your changes. **This is where things might get tricky.** If someone else edited the same file as you and pushed their changes before you did, this will generate a merge conflict. Open the file and you will see the lines that a different between your version and the online version marked with >>>>>>, ========, and <<<<<<< symbols. If this were code, you would delete the bad code, and the symbols, and then push the changes. **We want to keep all the changes, so don't delete any content, just the >>>>>> ======== <<<<<<< symbols**, and then push the changes.
- Wait a minute or two for the site to update, and check what it looks like at [https://ndm736.github.io/ME433.Kitchen/](https://ndm736.github.io/ME433.Kitchen/)
- If necessary, continue to edit and push your changes until you are done.

This exercise is kind of an experiment, it is inevitable that someone accidentally deletes a huge section of other people's work. Don't worry if you do it, we can fix it. Normally on a project only a handful of people would have merging permissions, and people without would make a fork of the main project and a pull request when they want to push their changes into the main code. But this is a learning experience, so let's see how it goes.

In Canvas for the HW1 assignment, submit a link to the specific page for your recipe.