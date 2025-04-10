import pandas as pd

from experiments.aggregate import merge_results, generated_property_post_processing, type0_primary_sort, \
    type0_primary_label



def type0_aggregated_df():
    df = merge_results("./../experiments_no_mdd/type0", r'^(\d+)_(\d*n-div-\d+).(\d+).out$', None, None)
    generated_df = merge_results("./../experiments_no_mdd/generated_instances", r'^(\d+)_(\d+).(\d+).out$', None, None,
                                 generated_property_post_processing)
    df = pd.concat([df, generated_df], ignore_index=True).fillna("-")
    df["primary_sort"] = df["Property_2"].apply(type0_primary_sort)
    df["secondary_sort"] = df["Property_1"]
    df.sort_values(by=["primary_sort", "secondary_sort"], inplace=True)

    df["primary_label"] = df["Property_2"].apply(type0_primary_label)
    df["secondary_label"] = df["Property_1"]

    return df


def type1_aggregated_df():
    df = merge_results("./../experiments_no_mdd/type1", r'^(\d+)_(\d+)reps.(\d+).out$', None, None)

    df["primary_sort"] = df["Property_1"]
    df["secondary_sort"] = df["Property_2"]
    df.sort_values(by=["primary_sort", "secondary_sort"], inplace=True)

    df["primary_label"] = df["Property_1"]
    df["secondary_label"] = df["Property_2"]

    return df
