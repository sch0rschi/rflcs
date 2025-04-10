from experiments.aggregate import type0_df, type1_df
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import re


def div_fraction_format(category):
    pattern = r'(\d*)n-div-(\d+)'
    match = re.match(pattern, category)
    if match:
        top = match.group(1)
        bottom = match.group(2)
        if top == "":
            return "$\Sigma = \\frac{m}{" + str(bottom) + "}$"
        else:
            return "$\Sigma = \\frac{" + str(top) + "\\cdot m}{" + str(bottom) + "}$"


def repetitions_format(category):
    return category + " repetitions"

def box_plot(df, title, x_label, y_label, file_template, projection_1, projection_2, categories, selector_property, running_property,
             plot_label_format):
    selected_df_1 = df[projection_1]
    selected_df_2 = df[projection_2]

    for category in categories:
        # Filter rows based on category for both projections
        subset_rows_1 = selected_df_1[selected_df_1[selector_property] == category].reset_index(drop=True)
        subset_rows_1 = subset_rows_1.drop(columns=[selector_property])

        subset_rows_2 = selected_df_2[selected_df_2[selector_property] == category].reset_index(drop=True)
        subset_rows_2 = subset_rows_2.drop(columns=[selector_property])

        # Melt both dataframes
        df_melted_1 = subset_rows_1.melt(id_vars=[running_property], var_name='Statistic', value_name='Value')
        df_melted_2 = subset_rows_2.melt(id_vars=[running_property], var_name='Statistic', value_name='Value')

        # Add a new column to differentiate between the two melted dataframes
        df_melted_1['Variant'] = 'Match based ILP'
        df_melted_2['Variant'] = 'MDD based ILP'

        # Combine the two melted dataframes into one
        combined_df = pd.concat([df_melted_1, df_melted_2])

        # Plot both sets side by side using 'Variant' as the hue
        plt.figure(figsize=(10, 6))  # You can adjust the figure size as needed
        sns.boxplot(x=running_property, y='Value', data=combined_df, hue='Variant', whis=[0, 100], showmeans=True, meanprops={"marker":"D", "markerfacecolor":"black", "markeredgecolor":"black"})

        plt.title(title + plot_label_format(category))
        plt.xlabel(x_label)
        plt.ylabel(y_label)
        plt.xticks(rotation=45)
        plt.grid(True)

        # Save the plot to file
        plt.savefig(file_template.replace("<>", category), format="svg", bbox_inches="tight")
        plt.close()


df0 = type0_df()
box_plot(df0,
         "No Mdd Runtime Statistics for ",
         "$m$",
         "Runtime [s]",
         "no_mdd_runtime_type0_<>.svg",
         ["Property_2", "Property_1", 'match_ilp_runtime'],
         ["Property_2", "Property_1", 'mdd_ilp_runtime'],
         ["n-div-8", "n-div-4", "3n-div-8", "n-div-2", "5n-div-8", "3n-div-4", "7n-div-8"],
         "Property_2",
         "Property_1",
         div_fraction_format
         )

box_plot(df0,
         "No Mdd Solution Length Statistics for ",
         "$m$",
         "RFLCS length",
         "no_mdd_solution_type0_<>.svg",
         ["Property_2", "Property_1", 'match_ilp_solution_length'],
         ["Property_2", "Property_1", 'mdd_ilp_solution_length'],
         ["n-div-8", "n-div-4", "3n-div-8", "n-div-2", "5n-div-8", "3n-div-4", "7n-div-8"],
         "Property_2",
         "Property_1",
         div_fraction_format
         )

df1 = type1_df()
box_plot(df1,
         "No Mdd Runtime Statistics for ",
         "$|\Sigma|$",
         "Runtime [s]",
         "no_mdd_runtime_type1_<>reps.svg",
         ["Property_2", "Property_1", 'match_ilp_runtime'],
         ["Property_2", "Property_1", 'mdd_ilp_runtime'],
         ["3", "4", "5", "6", "7", "8"],
         "Property_2",
         "Property_1",
         repetitions_format
         )


box_plot(df1,
         "No Mdd Solution Length Statistics for ",
         "$|\Sigma|$",
         "RFLCS length",
         "no_mdd_solution_type1_<>reps.svg",
         ["Property_2", "Property_1", 'match_ilp_solution_length'],
         ["Property_2", "Property_1", 'mdd_ilp_solution_length'],
         ["3", "4", "5", "6", "7", "8"],
         "Property_2",
         "Property_1",
         repetitions_format
         )
