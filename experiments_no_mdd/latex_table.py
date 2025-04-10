from aggregate import type0_aggregated_df, type1_aggregated_df
from experiments.formatting import format_max, clamp_format_min, format_min


def create_table(df,
                 filename,
                 caption,
                 label,
                 heading_property_1,
                 heading_property_2):
    table_prefix = ('''\\begin{table}[!ht]
    \\centering
    \\caption{<caption>}
    \\label{<label>}
    \\resizebox{0.7\\textwidth}{!}{
    \\begin{tabular}{c|c|ccc|ccc}
    \\hline
    \\multirow{2}{*}{<heading_property_1>} & \\multirow{2}{*}{<heading_property_2>}
    & \\multicolumn{3}{c|}{Match ILP}
    & \\multicolumn{3}{c}{MDD ILP} \\\\
    \\cline{3-8}
    &
    & \\multicolumn{1}{r|}{obj}
    & \\multicolumn{1}{r|}{$t_{ilp}${[}s{]}} 
    & \\multicolumn{1}{r|}{gap{[}\\%{]}} 
    
    & \\multicolumn{1}{r|}{obj}
    & \\multicolumn{1}{r|}{$t_{ilp}${[}s{]}} 
    & \\multicolumn{1}{r}{gap{[}\\%{]}} \\\\
    '''
                    .replace("<caption>", caption)
                    .replace("<label>", label)
                    .replace("<heading_property_1>", heading_property_1)
                    .replace("<heading_property_2>", heading_property_2))

    table_postfix = '''\\end{tabular}
        }
    \\end{table}'''

    group_prefix = '''\\multirow{<rows>}{*}{<primary_label>}'''

    group_postfix = ''' \\hline'''

    group = '''& <secondary_label> & \\multicolumn{1}{r|}{<obj_match>} & \\multicolumn{1}{r|}{<t_match>} & \\multicolumn{1}{r|}{<gap_match>} & \\multicolumn{1}{r|}{<obj_mdd>} & \\multicolumn{1}{r|}{<t_mdd>} & \\multicolumn{1}{r}{<gap_mdd>} \\\\'''

    table_data = table_prefix + '\n'

    frac = ''

    for index, row in df.iterrows():
        if frac != row["primary_label"]:
            frac = row["primary_label"]
            table_data += group_postfix + '\n'
            table_data += group_prefix.replace("<rows>", str(df.loc[df["primary_label"] == frac].shape[0])).replace('<primary_label>', str(row["primary_label"])) + '\n'
        table_data += (group
                       .replace('<secondary_label>', str(row["secondary_label"]))
                       .replace('<obj_match>', format_max(row['avg_match_ilp_solution_length'],
                                                          [row['avg_mdd_ilp_solution_length']]))
                       .replace('<gap_match>',
                                format_min(row['avg_match_ilp_gap'], [row['avg_mdd_ilp_gap']]))
                       .replace('<t_match>',
                                clamp_format_min(row['avg_match_ilp_runtime'], [row['avg_mdd_ilp_runtime']]))
                       .replace('<obj_mdd>',
                                format_max(row['avg_mdd_ilp_solution_length'],
                                           [row['avg_match_ilp_solution_length']]))
                       .replace('<gap_mdd>',
                                format_min(row['avg_mdd_ilp_gap'], [row['avg_match_ilp_gap']]))
                       .replace('<t_mdd>', clamp_format_min(row['avg_mdd_ilp_runtime'], [row['avg_match_ilp_runtime']]))
                       + '\n')

    table_data += group_postfix + '\n'
    table_data += table_postfix + '\n'

    with open(filename, 'w') as file:
        file.write(table_data)

df = type0_aggregated_df()
create_table(df,
             "table_0_no_mdd.tex",
             "No MDD Instance Set 1 Results",
             "tab:results_1_no_mdd",
             "$|\\Sigma|$",
             "$n$"
             )


df = type1_aggregated_df()
create_table(df,
             "table_1_no_mdd.tex",
             "No MDD Instance Set 2 Results",
             "tab:results_2_no_mdd",
             "$|\\Sigma|$",
             "reps",
             )
