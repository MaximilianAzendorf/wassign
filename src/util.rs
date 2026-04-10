use std::time::{Duration, SystemTime};

pub fn str_float(value: f64, precision: usize) -> String {
    format!("{value:.precision$}")
}

pub fn time_now() -> SystemTime {
    SystemTime::now()
}

pub fn time_never() -> SystemTime {
    SystemTime::now() + Duration::from_secs(3_153_600_000)
}

pub fn riffle_shuffle<T>(left: Vec<T>, right: Vec<T>, rng: &mut crate::Rng) -> Vec<T> {
    let left_len = left.len();
    let right_len = right.len();
    let mut pattern = vec![0_u8; left_len + right_len];
    pattern[left_len..].fill(1);

    rng.shuffle(&mut pattern);

    let mut left_iter = left.into_iter();
    let mut right_iter = right.into_iter();

    pattern
        .into_iter()
        .map(|marker| {
            if marker == 0 {
                left_iter.next().expect("left pattern length must match")
            } else {
                right_iter.next().expect("right pattern length must match")
            }
        })
        .collect()
}

#[cfg(test)]
fn format_duration(duration: Duration) -> String {
    let total = duration.as_secs();
    let hours = total / 3600;
    let minutes = (total % 3600) / 60;
    let seconds = total % 60;

    format!("{hours:02}:{minutes:02}:{seconds:02}")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn str_should_work() {
        assert_eq!(5_i32.to_string(), "5");
        assert_eq!(5_u32.to_string(), "5");
        assert!(5.5_f32.to_string().starts_with("5.5"));
        assert!(5.5_f64.to_string().starts_with("5.5"));
        assert_eq!(str_float(5.124, 2), "5.12");
        assert_eq!(format_duration(Duration::from_secs(5)), "00:00:05");
        assert_eq!(
            format_duration(Duration::from_secs(3 * 3600 + 26 * 60 + 53)),
            "03:26:53"
        );
        assert_eq!(format_duration(Duration::from_millis(61_100)), "00:01:01");
        assert_eq!(
            format_duration(Duration::from_nanos(123_000_000_000)),
            "00:02:03"
        );
    }

    #[test]
    fn riffle_shuffle_should_work() {
        const SIZE: i32 = 10;

        let v1 = (0..SIZE).collect::<Vec<_>>();
        let v2 = (SIZE..(SIZE * 2)).collect::<Vec<_>>();
        let mut rng = crate::Rng::from_seed(12);
        let res = riffle_shuffle(v1.clone(), v2.clone(), &mut rng);

        let mut v1b = Vec::new();
        let mut v2b = Vec::new();

        for value in res {
            if value < SIZE {
                v1b.push(value);
            } else {
                v2b.push(value);
            }
        }

        assert_eq!(v1, v1b);
        assert_eq!(v2, v2b);
    }
}
