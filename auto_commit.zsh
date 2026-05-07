#!/bin/zsh

# ============================================================
# Git Commit & Changelog 自动生成脚本
# 用法: git-commit-changelog <type> <description>
# 示例: git-commit-changelog feat "添加行为树决策节点"
# ============================================================

# 颜色定义
autoload -U colors && colors
RED="%{$fg[red]%}"
GREEN="%{$fg[green]%}"
YELLOW="%{$fg[yellow]%}"
BLUE="%{$fg[blue]%}"
RESET="%{$reset_color%}"

# 配置
PROJECT_ROOT="${PROJECT_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null)}"
CHANGELOG_FILE="${CHANGELOG_FILE:-CHANGELOG.md}"
TEMP_FILE="/tmp/changelog_temp_$$"

# 提交类型及其说明
typeset -A TYPES
TYPES=(
    "feat"     "✨ 新功能 (New Feature)"
    "fix"      "🐛 Bug修复 (Bug Fix)"
    "docs"     "📚 文档 (Documentation)"
    "style"    "💎 代码格式 (Code Style)"
    "refactor" "🔄 代码重构 (Refactor)"
    "perf"     "⚡ 性能优化 (Performance)"
    "test"     "✅ 测试 (Testing)"
    "chore"    "🔧 杂务/构建 (Chore/Build)"
    "ci"       "🤖 CI配置 (CI Configuration)"
    "revert"   "⏪ 回滚 (Revert)"
)

# ============================================================
# 辅助函数
# ============================================================

# 显示帮助信息
show_help() {
    cat << EOF
${BLUE}Git Commit & Changelog 自动生成脚本${RESET}

${YELLOW}用法:${RESET}
  $0 <type> <description>
  $0 --changelog-only
  $0 --help

${YELLOW}提交类型:${RESET}
EOF
    for type in ${(k)TYPES}; do
        printf "  %-8s %s\n" "$type" "${TYPES[$type]}"
    done
    cat << EOF

${YELLOW}示例:${RESET}
  $0 feat "添加行为树决策节点"
  $0 fix "修复目标点发布错误"
  $0 docs "更新README"
  $0 chore "添加clang-format配置"
  $0 --changelog-only  # 只重新生成changelog

${YELLOW}注意:${RESET}
  - 请确保在git仓库根目录下运行
  - commit message 会遵循约定式提交规范
  - changelog 会自动更新
EOF
}

# 显示错误并退出
error_exit() {
    echo "${RED}错误: $1${RESET}" >&2
    exit 1
}

# 显示成功信息
success_msg() {
    echo "${GREEN}✓ $1${RESET}"
}

# 显示信息
info_msg() {
    echo "${BLUE}ℹ $1${RESET}"
}

# 获取当前版本（从最新的git tag）
get_current_version() {
    local version=$(git describe --tags --abbrev=0 2>/dev/null)
    if [[ -z "$version" ]]; then
        echo "v0.0.0"
    else
        echo "$version"
    fi
}

# 获取下一个版本号（基于commit类型自动建议）
suggest_next_version() {
    local current=$(get_current_version)
    local major=$(echo $current | cut -d. -f1 | sed 's/v//')
    local minor=$(echo $current | cut -d. -f2)
    local patch=$(echo $current | cut -d. -f3)
    
    # 根据最近的commit类型建议版本号
    local last_commits=$(git log -10 --pretty=format:%s)
    if echo "$last_commits" | grep -q "^feat.*!"; then
        # 破坏性变更，增加major版本
        echo "v$((major + 1)).0.0"
    elif echo "$last_commits" | grep -q "^feat"; then
        # 新功能，增加minor版本
        echo "v${major}.$((minor + 1)).0"
    else
        # 修复或杂务，增加patch版本
        echo "v${major}.${minor}.$((patch + 1))"
    fi
}

# 解析commit message
parse_commit_message() {
    local msg="$1"
    local type=$(echo "$msg" | cut -d: -f1 | xargs)
    local desc=$(echo "$msg" | cut -d: -f2- | xargs)
    
    # 检查是否有scope
    if [[ "$type" =~ ^[a-z]+\(.+\)$ ]]; then
        local scope=$(echo "$type" | sed 's/^[a-z]*(\(.*\))$/\1/')
        type=$(echo "$type" | sed 's/(.*$//')
    else
        local scope=""
    fi
    
    echo "$type|$scope|$desc"
}

# ============================================================
# Changelog 生成函数
# ============================================================

# 生成单个版本的changelog
generate_version_changelog() {
    local tag="$1"
    local prev_tag="$2"
    local date="$3"
    local output_file="$4"
    
    # 获取该版本范围内的所有commit
    local commits=""
    if [[ -z "$prev_tag" ]]; then
        commits=$(git log --pretty=format:"%s|%h|%an" "$tag" 2>/dev/null)
    else
        commits=$(git log --pretty=format:"%s|%h|%an" "${prev_tag}..${tag}" 2>/dev/null)
    fi
    
    if [[ -z "$commits" ]]; then
        return
    fi
    
    # 初始化分类
    local added=()
    local fixed=()
    local changed=()
    local docs=()
    local maintenance=()
    local other=()
    
    while IFS= read -r line; do
        [[ -z "$line" ]] && continue
        local msg=$(echo "$line" | cut -d'|' -f1)
        local hash=$(echo "$line" | cut -d'|' -f2)
        local author=$(echo "$line" | cut -d'|' -f3)
        
        local type=""
        local desc=""
        if [[ "$msg" =~ ^[a-z]+: ]]; then
            type=$(echo "$msg" | cut -d: -f1)
            desc=$(echo "$msg" | cut -d: -f2- | xargs)
        else
            type="other"
            desc="$msg"
        fi
        
        local entry="- ${desc} (${hash})"
        
        case "$type" in
            feat)   added+=("$entry") ;;
            fix)    fixed+=("$entry") ;;
            docs)   docs+=("$entry") ;;
            chore|ci) maintenance+=("$entry") ;;
            refactor|perf) changed+=("$entry") ;;
            *)      other+=("$entry") ;;
        esac
    done <<< "$commits"
    
    # 写入changelog
    {
        echo "## [$tag] - $date"
        echo ""
        
        [[ ${#added[@]} -gt 0 ]] && {
            echo "### ✨ 新功能 (Added)"
            printf '%s\n' "${added[@]}"
            echo ""
        }
        
        [[ ${#fixed[@]} -gt 0 ]] && {
            echo "### 🐛 Bug 修复 (Fixed)"
            printf '%s\n' "${fixed[@]}"
            echo ""
        }
        
        [[ ${#changed[@]} -gt 0 ]] && {
            echo "### 🔄 变更/重构 (Changed)"
            printf '%s\n' "${changed[@]}"
            echo ""
        }
        
        [[ ${#docs[@]} -gt 0 ]] && {
            echo "### 📚 文档 (Documentation)"
            printf '%s\n' "${docs[@]}"
            echo ""
        }
        
        [[ ${#maintenance[@]} -gt 0 ]] && {
            echo "### 🔧 维护/构建 (Maintenance)"
            printf '%s\n' "${maintenance[@]}"
            echo ""
        }
        
        [[ ${#other[@]} -gt 0 ]] && {
            echo "### 📦 其他 (Other)"
            printf '%s\n' "${other[@]}"
            echo ""
        }
    } >> "$output_file"
}

# 主函数：生成完整的changelog
generate_changelog() {
    info_msg "正在生成 CHANGELOG.md ..."
    
    # 获取所有tags（按版本排序）
    local tags=($(git tag --sort=version:refname 2>/dev/null))
    
    # 如果没有tags，只显示未发布的变更
    if [[ ${#tags[@]} -eq 0 ]]; then
        {
            echo "# Changelog"
            echo ""
            echo "## [Unreleased] - $(date +%Y-%m-%d)"
            echo ""
        } > "$CHANGELOG_FILE"
        generate_version_changelog "HEAD" "" "$(date +%Y-%m-%d)" "$CHANGELOG_FILE"
        success_msg "CHANGELOG.md 已生成（无版本标签）"
        return
    fi
    
    # 生成完整的changelog
    {
        echo "# Changelog"
        echo ""
        echo "所有显著变更都将记录在此文件中。"
        echo ""
        echo "格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，"
        echo "版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。"
        echo ""
    } > "$CHANGELOG_FILE"
    
    # 生成每个版本的changelog
    local prev_tag=""
    for tag in "${tags[@]}"; do
        local tag_date=$(git log -1 --format=%ai "$tag" | cut -d' ' -f1)
        generate_version_changelog "$tag" "$prev_tag" "$tag_date" "$CHANGELOG_FILE"
        prev_tag="$tag"
    done
    
    # 添加未发布的变更
    {
        echo "## [Unreleased]"
        echo ""
        echo "### 即将到来"
        echo "- 待添加的变更..."
        echo ""
    } >> "$CHANGELOG_FILE"
    
    success_msg "CHANGELOG.md 已生成（包含 ${#tags[@]} 个版本）"
}

# ============================================================
# 主函数：提交并生成changelog
# ============================================================

main() {
    # 检查是否在git仓库中
    if ! git rev-parse --git-dir > /dev/null 2>&1; then
        error_exit "当前目录不是git仓库"
    fi
    
    PROJECT_ROOT=$(git rev-parse --show-toplevel)
    cd "$PROJECT_ROOT"
    
    # 处理特殊参数
    case "$1" in
        --help|-h)
            show_help
            exit 0
            ;;
        --changelog-only)
            generate_changelog
            # 如果changelog有变化，自动提交
            if ! git diff --quiet "$CHANGELOG_FILE" 2>/dev/null; then
                git add "$CHANGELOG_FILE"
                git commit -m "docs: 更新 CHANGELOG.md"
                success_msg "已提交 CHANGELOG.md 更新"
            fi
            exit 0
            ;;
    esac
    
    # 获取提交参数
    local commit_type="$1"
    local commit_desc="$2"
    local commit_scope="$3"
    
    # 验证参数
    if [[ -z "$commit_type" ]] || [[ -z "$commit_desc" ]]; then
        error_exit "请提供提交类型和描述\n用法: $0 <type> <description>"
    fi
    
    # 验证提交类型
    if [[ -z "${TYPES[$commit_type]}" ]]; then
        error_exit "无效的提交类型: $commit_type\n可用类型: ${(j:, :)${(k)TYPES}}"
    fi
    
    # 构建完整的commit message
    local full_msg=""
    if [[ -n "$commit_scope" ]]; then
        full_msg="${commit_type}(${commit_scope}): ${commit_desc}"
    else
        full_msg="${commit_type}: ${commit_desc}"
    fi
    
    info_msg "准备提交: ${GREEN}$full_msg${RESET}"
    
    # 检查是否有文件需要提交
    if git diff --quiet && git diff --cached --quiet; then
        error_exit "没有需要提交的变更，请先执行 git add"
    fi
    
    # 执行git提交
    git commit -m "$full_msg"
    if [[ $? -ne 0 ]]; then
        error_exit "git commit 失败"
    fi
    success_msg "提交成功"
    
    # 获取新提交的hash
    local commit_hash=$(git rev-parse --short HEAD)
    
    # 询问是否推送
    echo -n "${YELLOW}是否推送到远程？(y/N): ${RESET}"
    read -r should_push
    if [[ "$should_push" =~ ^[Yy]$ ]]; then
        local current_branch=$(git branch --show-current)
        git push origin "$current_branch"
        success_msg "已推送到远程"
    fi
    
    # 询问是否生成changelog
    echo -n "${YELLOW}是否更新CHANGELOG？(Y/n): ${RESET}"
    read -r should_changelog
    if [[ "$should_changelog" =~ ^[Nn]$ ]]; then
        info_msg "跳过changelog生成"
    else
        generate_changelog
        
        # 提交changelog
        if ! git diff --quiet "$CHANGELOG_FILE" 2>/dev/null; then
            git add "$CHANGELOG_FILE"
            git commit -m "docs: 更新 CHANGELOG.md"
            success_msg "已提交 CHANGELOG.md"
            
            # 询问是否推送changelog
            echo -n "${YELLOW}是否推送changelog的更新？(y/N): ${RESET}"
            read -r push_changelog
            if [[ "$push_changelog" =~ ^[Yy]$ ]]; then
                local current_branch=$(git branch --show-current)
                git push origin "$current_branch"
                success_msg "已推送changelog更新"
            fi
        fi
    fi
    
    # 显示最后3条提交记录
    echo ""
    info_msg "最近提交:"
    git log --oneline -3
}

# 执行主函数
main "$@" 