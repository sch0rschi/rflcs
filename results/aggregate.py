import glob
import os
import re

import pandas as pd


def parse_experiment_file(file_path):
    metrics = {
        "solved": 0,
        "solution_length": 0.0,
        "upper_bound": 0.0,
        "solution_runtime": 0.0,
        "heuristic_solution_length": 0.0,
        "heuristic_solution_runtime": 0.0,
        "heuristic_runtime": 0.0,
        "reduction_is_complete": 0,
        "reduction_quality": 0.0,
        "reduction_upper_bound": 0.0,
        "reduction_runtime": 0.0,
        "match_ilp_solution_length": 0.0,
        "match_ilp_upper_bound": 0.0,
        "match_ilp_runtime": 0.0,
        "mdd_ilp_solution_length": 0.0,
        "mdd_ilp_upper_bound": 0,
        "mdd_ilp_runtime": 0.0,
        "match_ilp_gap": 0.0,
        "mdd_ilp_gap": 0.0,
        "mdd_memory_consumption": 0.0,
        "main_process_memory_consumption": 0.0,
        "heuristic_solution_best": 0,
    }

    with open(file_path, 'r', encoding='utf-8') as f:
        data = f.readlines()

    for line in data:
        key_value = re.split(r'\s*:\s*', line.strip(), maxsplit=1)  # Handle any whitespace around ':'

        if len(key_value) != 2:
            continue

        key, value = key_value

        if key == "solved" and value.lower() == "true":
            metrics["solved"] = 1
        elif key == "reduction_is_complete" and value.lower() == "true":
            metrics["reduction_is_complete"] = 1
        elif key == "reduction_quality":
            metrics["reduction_quality"] = 100 * float(value)
        elif key == "mdd_memory_consumption":
            metrics["mdd_memory_consumption"] = float(value) / 1000
        elif key in metrics:
            try:
                metrics[key] = float(value)
            except ValueError:
                pass

    metrics["gap"] = 100 * (metrics["upper_bound"] - metrics["solution_length"]) / metrics["upper_bound"]
    metrics["match_ilp_gap"] = 100 * (metrics["match_ilp_upper_bound"] - metrics["match_ilp_solution_length"]) / \
                               metrics["match_ilp_upper_bound"]
    metrics["mdd_ilp_gap"] = 100 * (metrics["mdd_ilp_upper_bound"] - metrics["mdd_ilp_solution_length"]) / metrics[
        "mdd_ilp_upper_bound"]
    if metrics["solution_length"] == metrics["heuristic_solution_length"]:
        metrics["heuristic_solution_best"] = 1

    return metrics


def aggregate_results(type_folder, splitter_regex, post_processing=None):
    df = create_df(type_folder, splitter_regex, post_processing)

    # Replace negative values in "match_ilp_upper_bound" with values from "reduction_upper_bound"
    df["match_ilp_upper_bound"] = df.apply(
        lambda row: row["reduction_upper_bound"] if row["match_ilp_upper_bound"] < 0 else row["match_ilp_upper_bound"],
        axis=1)
    df["mdd_ilp_upper_bound"] = df.apply(
        lambda row: row["reduction_upper_bound"] if row["mdd_ilp_upper_bound"] < 0 else row["mdd_ilp_upper_bound"],
        axis=1)

    # Group by Length and Category, then aggregate
    aggregated_df = df.groupby(["Property_1", "Property_2"], as_index=False).agg(
        solved_count=("solved", "sum"),
        reduction_complete_count=("reduction_is_complete", "sum"),
        heuristic_best_count=("heuristic_solution_best", "sum"),

        avg_solution_length=("solution_length", "mean"),
        median_solution_length=("solution_length", "median"),
        min_solution_length=("solution_length", "min"),
        max_solution_length=("solution_length", "max"),
        q25_solution_length=("solution_length", lambda x: x.quantile(0.25)),
        q75_solution_length=("solution_length", lambda x: x.quantile(0.75)),

        avg_heuristic_solution_length=("heuristic_solution_length", "mean"),
        median_heuristic_solution_length=("heuristic_solution_length", "median"),
        min_heuristic_solution_length=("heuristic_solution_length", "min"),
        max_heuristic_solution_length=("heuristic_solution_length", "max"),
        q25_heuristic_solution_length=("heuristic_solution_length", lambda x: x.quantile(0.25)),
        q75_heuristic_solution_length=("heuristic_solution_length", lambda x: x.quantile(0.75)),

        avg_reduction_quality=("reduction_quality", "mean"),
        median_reduction_quality=("reduction_quality", "median"),
        min_reduction_quality=("reduction_quality", "min"),
        max_reduction_quality=("reduction_quality", "max"),
        q25_reduction_quality=("reduction_quality", lambda x: x.quantile(0.25)),
        q75_reduction_quality=("reduction_quality", lambda x: x.quantile(0.75)),

        avg_upper_bound=("upper_bound", "mean"),
        median_upper_bound=("upper_bound", "median"),
        min_upper_bound=("upper_bound", "min"),
        max_upper_bound=("upper_bound", "max"),
        q25_upper_bound=("upper_bound", lambda x: x.quantile(0.25)),
        q75_upper_bound=("upper_bound", lambda x: x.quantile(0.75)),

        avg_solution_runtime=("solution_runtime", "mean"),
        median_solution_runtime=("solution_runtime", "median"),
        min_solution_runtime=("solution_runtime", "min"),
        max_solution_runtime=("solution_runtime", "max"),
        q25_solution_runtime=("solution_runtime", lambda x: x.quantile(0.25)),
        q75_solution_runtime=("solution_runtime", lambda x: x.quantile(0.75)),

        avg_heuristic_solution_runtime=("heuristic_solution_runtime", "mean"),
        median_heuristic_solution_runtime=("heuristic_solution_runtime", "median"),
        min_heuristic_solution_runtime=("heuristic_solution_runtime", "min"),
        max_heuristic_solution_runtime=("heuristic_solution_runtime", "max"),
        q25_heuristic_solution_runtime=("heuristic_solution_runtime", lambda x: x.quantile(0.25)),
        q75_heuristic_solution_runtime=("heuristic_solution_runtime", lambda x: x.quantile(0.75)),

        avg_heuristic_runtime=("heuristic_runtime", "mean"),
        median_heuristic_runtime=("heuristic_runtime", "median"),
        min_heuristic_runtime=("heuristic_runtime", "min"),
        max_heuristic_runtime=("heuristic_runtime", "max"),
        q25_heuristic_runtime=("heuristic_runtime", lambda x: x.quantile(0.25)),
        q75_heuristic_runtime=("heuristic_runtime", lambda x: x.quantile(0.75)),

        avg_reduction_upper_bound=("reduction_upper_bound", "mean"),
        median_reduction_upper_bound=("reduction_upper_bound", "median"),
        min_reduction_upper_bound=("reduction_upper_bound", "min"),
        max_reduction_upper_bound=("reduction_upper_bound", "max"),
        q25_reduction_upper_bound=("reduction_upper_bound", lambda x: x.quantile(0.25)),
        q75_reduction_upper_bound=("reduction_upper_bound", lambda x: x.quantile(0.75)),

        avg_reduction_runtime=("reduction_runtime", "mean"),
        median_reduction_runtime=("reduction_runtime", "median"),
        min_reduction_runtime=("reduction_runtime", "min"),
        max_reduction_runtime=("reduction_runtime", "max"),
        q25_reduction_runtime=("reduction_runtime", lambda x: x.quantile(0.25)),
        q75_reduction_runtime=("reduction_runtime", lambda x: x.quantile(0.75)),

        avg_match_ilp_solution_length=("match_ilp_solution_length", "mean"),
        median_match_ilp_solution_length=("match_ilp_solution_length", "median"),
        min_match_ilp_solution_length=("match_ilp_solution_length", "min"),
        max_match_ilp_solution_length=("match_ilp_solution_length", "max"),
        q25_match_ilp_solution_length=("match_ilp_solution_length", lambda x: x.quantile(0.25)),
        q75_match_ilp_solution_length=("match_ilp_solution_length", lambda x: x.quantile(0.75)),

        avg_match_ilp_upper_bound=("match_ilp_upper_bound", "mean"),
        median_match_ilp_upper_bound=("match_ilp_upper_bound", "median"),
        min_match_ilp_upper_bound=("match_ilp_upper_bound", "min"),
        max_match_ilp_upper_bound=("match_ilp_upper_bound", "max"),
        q25_match_ilp_upper_bound=("match_ilp_upper_bound", lambda x: x.quantile(0.25)),
        q75_match_ilp_upper_bound=("match_ilp_upper_bound", lambda x: x.quantile(0.75)),

        avg_match_ilp_runtime=("match_ilp_runtime", "mean"),
        median_match_ilp_runtime=("match_ilp_runtime", "median"),
        min_match_ilp_runtime=("match_ilp_runtime", "min"),
        max_match_ilp_runtime=("match_ilp_runtime", "max"),
        q25_match_ilp_runtime=("match_ilp_runtime", lambda x: x.quantile(0.25)),
        q75_match_ilp_runtime=("match_ilp_runtime", lambda x: x.quantile(0.75)),

        avg_mdd_ilp_solution_length=("mdd_ilp_solution_length", "mean"),
        median_mdd_ilp_solution_length=("mdd_ilp_solution_length", "median"),
        min_mdd_ilp_solution_length=("mdd_ilp_solution_length", "min"),
        max_mdd_ilp_solution_length=("mdd_ilp_solution_length", "max"),
        q25_mdd_ilp_solution_length=("mdd_ilp_solution_length", lambda x: x.quantile(0.25)),
        q75_mdd_ilp_solution_length=("mdd_ilp_solution_length", lambda x: x.quantile(0.75)),

        avg_mdd_ilp_upper_bound=("mdd_ilp_upper_bound", "mean"),
        median_mdd_ilp_upper_bound=("mdd_ilp_upper_bound", "median"),
        min_mdd_ilp_upper_bound=("mdd_ilp_upper_bound", "min"),
        max_mdd_ilp_upper_bound=("mdd_ilp_upper_bound", "max"),
        q25_mdd_ilp_upper_bound=("mdd_ilp_upper_bound", lambda x: x.quantile(0.25)),
        q75_mdd_ilp_upper_bound=("mdd_ilp_upper_bound", lambda x: x.quantile(0.75)),

        avg_mdd_ilp_runtime=("mdd_ilp_runtime", "mean"),
        median_mdd_ilp_runtime=("mdd_ilp_runtime", "median"),
        min_mdd_ilp_runtime=("mdd_ilp_runtime", "min"),
        max_mdd_ilp_runtime=("mdd_ilp_runtime", "max"),
        q25_mdd_ilp_runtime=("mdd_ilp_runtime", lambda x: x.quantile(0.25)),
        q75_mdd_ilp_runtime=("mdd_ilp_runtime", lambda x: x.quantile(0.75)),

        avg_gap=("gap", "mean"),
        median_gap=("gap", "median"),
        min_gap=("gap", "min"),
        max_gap=("gap", "max"),
        q25_gap=("gap", lambda x: x.quantile(0.25)),
        q75_gap=("gap", lambda x: x.quantile(0.75)),

        avg_match_ilp_gap=("match_ilp_gap", "mean"),
        median_match_ilp_gap=("match_ilp_gap", "median"),
        min_match_ilp_gap=("match_ilp_gap", "min"),
        max_match_ilp_gap=("match_ilp_gap", "max"),
        q25_match_ilp_gap=("match_ilp_gap", lambda x: x.quantile(0.25)),
        q75_match_ilp_gap=("match_ilp_gap", lambda x: x.quantile(0.75)),

        avg_mdd_ilp_gap=("mdd_ilp_gap", "mean"),
        median_mdd_ilp_gap=("mdd_ilp_gap", "median"),
        min_mdd_ilp_gap=("mdd_ilp_gap", "min"),
        max_mdd_ilp_gap=("mdd_ilp_gap", "max"),
        q25_mdd_ilp_gap=("mdd_ilp_gap", lambda x: x.quantile(0.25)),
        q75_mdd_ilp_gap=("mdd_ilp_gap", lambda x: x.quantile(0.75)),

        avg_mdd_memory_consumption=("mdd_memory_consumption", "mean"),
        median_mdd_memory_consumption=("mdd_memory_consumption", "median"),
        min_mdd_memory_consumption=("mdd_memory_consumption", "min"),
        max_mdd_memory_consumption=("mdd_memory_consumption", "max"),
        q25_mdd_memory_consumption=("mdd_memory_consumption", lambda x: x.quantile(0.25)),
        q75_mdd_memory_consumption=("mdd_memory_consumption", lambda x: x.quantile(0.75)),
    )

    return aggregated_df


def create_df(type_folder, splitter_regex, post_processing):
    results = []
    script_dir = os.path.dirname(os.path.abspath(__file__))
    directory = os.path.join(script_dir, type_folder)
    file_pattern = os.path.join(directory, "*.out")
    for file_path in glob.glob(file_pattern):
        match = re.search(splitter_regex, os.path.basename(file_path))
        if not match:
            continue

        property_1, property_2 = int(match.group(1)), match.group(2)
        metrics = parse_experiment_file(file_path)
        metrics["Property_1"] = property_1
        metrics["Property_2"] = property_2
        if post_processing is not None:
            metrics["Property_1"], metrics["Property_2"] = post_processing(property_1, property_2)

        results.append(metrics)
    df = pd.DataFrame(results)
    return df


def load_horn_table(filename, splitter_regex):
    """Loads additional data from type0_horn and merges it with the aggregated results."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    horn_table_path = os.path.join(script_dir, filename)

    if not os.path.exists(horn_table_path):
        print("Horn table file not found!")
        return pd.DataFrame()

    horn_df = pd.read_csv(horn_table_path, delimiter='\t')  # Assuming tab-separated file

    # Extract Length and Category from Filename column
    horn_df[["Property_1", "Property_2"]] = horn_df['horn_Filename'].str.extract(splitter_regex)
    horn_df["Property_1"] = horn_df["Property_1"].astype(int)

    return horn_df.drop(columns=['horn_Filename'])


def merge_results(result_folder, splitter_regex, horn_result_file=None, horn_splitter_regex=None,
                  post_processing=None):
    aggregated_df = aggregate_results(result_folder, splitter_regex, post_processing)

    if horn_result_file is not None:
        horn_df = load_horn_table(horn_result_file, horn_splitter_regex)
        merged_df = pd.merge(aggregated_df, horn_df, on=["Property_1", "Property_2"])
    else:
        merged_df = aggregated_df

    excluded_columns = ["horn_opt_mdd", "horn_opt_tot", "horn_gap", "solved_count", "reduction_complete_count",
                        "avg_gap", "avg_match_ilp_gap", "avg_mdd_ilp_gap"]

    merged_df.loc[:, ~merged_df.columns.isin(excluded_columns)] = merged_df.loc[:,
                                                                  ~merged_df.columns.isin(excluded_columns)].map(
        lambda x: 0.009 if isinstance(x, (int, float)) and x < 0.01 else x
    )

    return merged_df


def type0_primary_sort(fraction_str):
    match = re.match(r'(\d+)n-div-(\d+)', fraction_str)
    if match:
        numerator = int(match.group(1))
        denominator = int(match.group(2))
        return numerator / denominator
    else:
        match = re.match(r'n-div-(\d+)', fraction_str)
        numerator = 1
        denominator = int(match.group(1))
        return numerator / denominator


def type0_primary_label(category):
    pattern = r'(\d*)n-div-(\d+)'
    match = re.match(pattern, category)
    if match:
        top = match.group(1)
        bottom = match.group(2)
        if top == "":
            top = "n"
        else:
            top = top + "\\cdot n"
        return rf'$\frac{{{top}}}{{{bottom}}}$'


def generated_property_post_processing(property_1, property_2):
    return property_1, "n-div-8"


def type0_df():
    df = create_df("type0", r'^(\d+)_(\d*n-div-\d+).(\d+).out$', None)
    generated_df = create_df("generated_instances", r'^(\d+)_(\d+).(\d+).out$', generated_property_post_processing)
    df = pd.concat([df, generated_df], ignore_index=True).fillna("-")
    df = pd.concat([df, generated_df], ignore_index=True).fillna("-")
    df["primary_sort"] = df["Property_2"].apply(type0_primary_sort)
    df["secondary_sort"] = df["Property_1"]
    df.sort_values(by=["primary_sort", "secondary_sort"], inplace=True)
    df["primary_label"] = df["Property_2"].apply(type0_primary_label)
    df["secondary_label"] = df["Property_1"]

    return df


def type0_aggregated_df():
    df = merge_results("type0", r'^(\d+)_(\d*n-div-\d+).(\d+).out$', "type0_horn", r'^(\d+)_(\S+)$')
    generated_df = merge_results("generated_instances", r'^(\d+)_(\d+).(\d+).out$', None, None,
                                 generated_property_post_processing)
    df = pd.concat([df, generated_df], ignore_index=True).fillna("-")
    df["primary_sort"] = df["Property_2"].apply(type0_primary_sort)
    df["secondary_sort"] = df["Property_1"]
    df.sort_values(by=["primary_sort", "secondary_sort"], inplace=True)

    df["primary_label"] = df["Property_2"].apply(type0_primary_label)
    df["secondary_label"] = df["Property_1"]

    return df


def type1_df():
    df = create_df("type1", r'^(\d+)_(\d+)reps.(\d+).out$', None)
    df["primary_sort"] = df["Property_1"]
    df["secondary_sort"] = df["Property_2"]
    df.sort_values(by=["primary_sort", "secondary_sort"], inplace=True)
    df["primary_label"] = df["Property_1"]
    df["secondary_label"] = df["Property_2"]
    return df


def type1_aggregated_df():
    df = merge_results("type1", r'^(\d+)_(\d+)reps.(\d+).out$', "type1_horn", r'^(\d+)_(\d+)reps$')

    df["primary_sort"] = df["Property_1"]
    df["secondary_sort"] = df["Property_2"]
    df.sort_values(by=["primary_sort", "secondary_sort"], inplace=True)

    df["primary_label"] = df["Property_1"]
    df["secondary_label"] = df["Property_2"]

    return df
