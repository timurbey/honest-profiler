/**
 * Copyright (c) 2014 Richard Warburton (richard.warburton@gmail.com)
 * <p>
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * <p>
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * <p>
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **/
package com.insightfullogic.honest_profiler.ports.javafx.controller;

import static com.insightfullogic.honest_profiler.ports.javafx.model.filter.FilterType.STRING;
import static com.insightfullogic.honest_profiler.ports.javafx.model.filter.FilterType.THREAD_SAMPLE;
import static com.insightfullogic.honest_profiler.ports.javafx.model.filter.FilterType.TIME_SHARE;
import static com.insightfullogic.honest_profiler.ports.javafx.util.DialogUtil.FILTER;
import static com.insightfullogic.honest_profiler.ports.javafx.view.Icon.COLLAPSE_16;
import static com.insightfullogic.honest_profiler.ports.javafx.view.Icon.EXPAND_16;
import static com.insightfullogic.honest_profiler.ports.javafx.view.Icon.FUNNEL_16;
import static com.insightfullogic.honest_profiler.ports.javafx.view.Icon.FUNNEL_ACTIVE_16;
import static com.insightfullogic.honest_profiler.ports.javafx.view.Icon.viewFor;
import static com.insightfullogic.honest_profiler.ports.javafx.view.Rendering.renderMethod;
import static com.insightfullogic.honest_profiler.ports.javafx.view.Rendering.renderPercentage;

import java.util.function.Function;

import com.insightfullogic.honest_profiler.core.filters.ProfileFilter;
import com.insightfullogic.honest_profiler.core.profiles.Profile;
import com.insightfullogic.honest_profiler.core.profiles.ProfileNode;
import com.insightfullogic.honest_profiler.ports.javafx.controller.filter.FilterDialogController;
import com.insightfullogic.honest_profiler.ports.javafx.model.ApplicationContext;
import com.insightfullogic.honest_profiler.ports.javafx.model.filter.FilterSpecification;
import com.insightfullogic.honest_profiler.ports.javafx.util.DialogUtil;
import com.insightfullogic.honest_profiler.ports.javafx.util.TreeUtil;
import com.insightfullogic.honest_profiler.ports.javafx.view.cell.ProfileNodeTreeTableCell;
import com.insightfullogic.honest_profiler.ports.javafx.view.cell.TreeViewCell;
import com.insightfullogic.honest_profiler.ports.javafx.view.tree.MethodNodeAdapter;
import com.insightfullogic.honest_profiler.ports.javafx.view.tree.RootNodeAdapter;
import com.insightfullogic.honest_profiler.ports.javafx.view.tree.ThreadNodeAdapter;

import javafx.beans.property.ObjectProperty;
import javafx.beans.property.ReadOnlyStringWrapper;
import javafx.beans.property.SimpleObjectProperty;
import javafx.beans.property.StringProperty;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.Tooltip;
import javafx.scene.control.TreeItem;
import javafx.scene.control.TreeTableColumn;
import javafx.scene.control.TreeTableColumn.CellDataFeatures;
import javafx.scene.control.TreeTableView;

public class TreeViewController extends ProfileViewController<Profile>
{
    @FXML
    private Button filterButton;
    @FXML
    private Button expandAllButton;
    @FXML
    private Button collapseAllButton;

    @FXML
    private TreeTableView<ProfileNode> treeView;
    @FXML
    private TreeTableColumn<ProfileNode, String> methodColumn;
    @FXML
    private TreeTableColumn<ProfileNode, ProfileNode> percentColumn;
    @FXML
    private TreeTableColumn<ProfileNode, String> totalColumn;
    @FXML
    private TreeTableColumn<ProfileNode, String> selfColumn;

    private FilterDialogController filterDialogController;
    private ObjectProperty<FilterSpecification> filterSpec;

    private ProfileFilter currentFilter;

    private RootNodeAdapter rootNode;

    @FXML
    private void initialize()
    {
        super.initialize(profileContext -> profileContext.profileProperty());

        info(filterButton, "Specify filters restricting the visible entries");
        info(expandAllButton, "Expand all trees");
        info(collapseAllButton, "Collapse all trees");
        info(
            treeView,
            "Shows the Threads in the profiled application. Right-click any node for expand/collapse and export options.");

        currentFilter = new ProfileFilter();

        filterSpec = new SimpleObjectProperty<>(new FilterSpecification());
        filterSpec.addListener((property, oldValue, newValue) ->
        {
            filterButton.setGraphic(
                newValue == null || !newValue.isFiltering() ? viewFor(FUNNEL_16)
                    : viewFor(FUNNEL_ACTIVE_16));
            currentFilter = new ProfileFilter(newValue.getFilters());
            refresh(getTarget());
        });

        rootNode = new RootNodeAdapter(filterSpec);

        filterDialogController = (FilterDialogController) DialogUtil
            .<FilterSpecification>createDialog(FILTER, "Specify Filters", false);
        filterDialogController.addAllowedFilterTypes(STRING, THREAD_SAMPLE, TIME_SHARE);

        filterButton.setGraphic(viewFor(FUNNEL_16));
        filterButton.setTooltip(new Tooltip("Specify filters"));
        filterButton
            .setOnAction(event -> filterSpec.set(filterDialogController.showAndWait().get()));

        expandAllButton.setGraphic(viewFor(EXPAND_16));
        expandAllButton.setTooltip(new Tooltip("Expand all threads"));
        expandAllButton.setOnAction(
            event -> treeView.getRoot().getChildren().stream().forEach(TreeUtil::expandFully));

        collapseAllButton.setGraphic(viewFor(COLLAPSE_16));
        collapseAllButton.setTooltip(new Tooltip("Collapse all threads"));
        collapseAllButton.setOnAction(
            event -> treeView.getRoot().getChildren().stream().forEach(TreeUtil::collapseFully));

        treeView.setRoot(rootNode);

        totalColumn.setCellValueFactory(data -> wrapDouble(data, ProfileNode::getTotalTimeShare));
        selfColumn.setCellValueFactory(data -> wrapDouble(data, ProfileNode::getSelfTimeShare));

        methodColumn.setCellFactory(column -> new ProfileNodeTreeTableCell());
        methodColumn.setCellValueFactory(data -> buildProfileNodeCell(data.getValue()));

        percentColumn.setCellFactory(param -> new TreeViewCell());
    }

    // Instance Accessors

    @Override
    public void setApplicationContext(ApplicationContext applicationContext)
    {
        super.setApplicationContext(applicationContext);
        filterDialogController.setApplicationContext(appCtx());
    }

    // Helper Methods

    private StringProperty buildProfileNodeCell(TreeItem<ProfileNode> treeItem)
    {
        String text = "";

        if (treeItem instanceof ThreadNodeAdapter)
        {
            ThreadNodeAdapter adapter = (ThreadNodeAdapter) treeItem;
            String name = adapter.getThreadName();

            StringBuilder builder = new StringBuilder("Thread").append(' ');
            builder.append(
                (adapter.getThreadId() < 0) ? "Unknown [" + adapter.getThreadId() + "]"
                    : adapter.getThreadId());

            builder.append(' ')
                .append((name == null || name.isEmpty()) ? "Unknown" : "[" + name + "]")
                .append(" (depth = ").append(adapter.getDepth()).append(", # samples = ")
                .append(adapter.getNrOfSamples()).append(")");

            text = builder.toString();
        }
        else if (treeItem instanceof MethodNodeAdapter)
        {
            text = renderMethod(treeItem.getValue().getFrameInfo());
        }

        return new ReadOnlyStringWrapper(text);
    }

    private StringProperty wrapDouble(CellDataFeatures<ProfileNode, String> data,
        Function<ProfileNode, Double> accessor)
    {
        return new ReadOnlyStringWrapper(
            data.getValue().getValue() != null
                ? renderPercentage(accessor.apply(data.getValue().getValue())) : "");
    }

    @Override
    protected void refresh(Profile profile)
    {
        if (profile == null)
        {
            return;
        }

        Profile newProfile = profile.copy();
        currentFilter.accept(newProfile);
        rootNode.update(newProfile.getTrees());
    }
}
