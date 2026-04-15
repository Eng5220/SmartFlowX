# Contributing to the repo

## Git Workflow

We use one protected `main` branch, all work done in short-lived feature branches.

---

### Branch Protection Rules

do a pull request before merging into `main`

Require at least **1 approving review**

Require branches to be **up to date** before merging

**No direct pushes** to `main` 

---

### First-Time Setup

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@domain.com"
git clone https://github.com/Eng5220/PowerNode.git
cd PowerNode
```

---

### Daily Workflow

```bash
# 1. Get latest main
git checkout main
git pull origin main

# 2. Create a feature branch
git checkout -b feature/your-description

# 3. Do your work, then commit
git add .
git commit -m "brief: what you did"

# 4. Push and open a Pull Request on GitHub
git push origin feature/your-description
```

Then open a **Pull Request** on GitHub → get 1 review → merge → delete branch.

---

### Branch Naming

| Type    | Pattern                       | Example                   |
| ------- | ----------------------------- | ------------------------- |
| Feature | `feature/short-description` | `feature/state-machine` |
| Fix     | `fix/short-description`     | `fix/interrupt-handler` |
| Docs    | `docs/short-description`    | `docs/update-readme`    |

---

### Quick Reference

| Task          | Command                          |
| ------------- | -------------------------------- |
| Check status  | `git status`                   |
| Stage all     | `git add .`                    |
| Commit        | `git commit -m "description"`  |
| Push branch   | `git push origin branch-name`  |
| Pull latest   | `git pull origin main`         |
| New branch    | `git checkout -b feature/name` |
| Delete branch | `git branch -d feature/name`   |

---

### Common Fixes

**Uncommitted changes when switching branches:**

```bash
git stash          # save temporarily
git stash pop      # restore later
```

**Branch is behind main:**

```bash
git checkout main && git pull origin main
git checkout your-branch
git merge main
```

---

### Pull Request Rules

1. Branch must be up to date with `main`
2. At least **1 team member** must approve
3. No merging your own PR without a review
4. Delete branch after merge
