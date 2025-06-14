/**
 * @file QmlUtils.js
 * @brief QML 通用工具函数集合
 * @date 2025-06-07
 */

/**
 * @brief 自动查找 StackView 组件
 * @param startItem 开始搜索的 Item，通常传入当前组件 (this 或 page)
 * @param depth     当前搜索深度，默认为 0
 * @param maxDepth  最大搜索深度，默认为 5
 * @return 找到的 StackView 对象，如果没找到返回 null
 */
function findStackView(startItem, depth = 0, maxDepth = 10) {
    if (!startItem || depth > maxDepth) {
        return null;
    }

    /* 检查当前项是否为 StackView */
    if (startItem.toString().indexOf("StackView") !== -1 ||
        (startItem.objectName && startItem.objectName === "stackView") ||
        (startItem.hasOwnProperty && startItem.hasOwnProperty("push")) &&
        (startItem.hasOwnProperty && startItem.hasOwnProperty("pop"))) {
        return startItem;
    }

    /* 向上查找父级 */
    if (startItem.parent) {
        var result = findStackView(startItem.parent, depth + 1, maxDepth);
        if (result) {
            return result;
        }
    }

    /* 向下查找子级 */
    if (startItem.children) {
        for (var i = 0; i < startItem.children.length; i++) {
            var result = findStackView(startItem.children[i], depth + 1, maxDepth);
            if (result) {
                return result;
            }
        }
    }

    return null;
}
