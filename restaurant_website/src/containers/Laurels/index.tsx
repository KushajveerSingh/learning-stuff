import styles from './Laurels.module.css';
import { SubHeading } from '../../components';
import data from '../../data';

interface AwardCardProps {
  award: {
    imgUrl: string;
    title: string;
    subtitle: string;
  };
}

const AwardCard = ({ award }: AwardCardProps) => {
  const { imgUrl, title, subtitle } = award;
  return (
    <div className={styles.app__laurels_awards_card}>
      <img src={imgUrl} alt="award" />
      <div className={styles.app__laurels_awards_card_content}>
        <p className="p__cormorant" style={{ color: '#DCCA87' }}>
          {title}
        </p>
        <p className="p__cormorant">{subtitle}</p>
      </div>
    </div>
  );
};

const Laurels = () => {
  return (
    <div className="app__bg app__wrapper section__padding" id="awards">
      <div className="app__wrapper_info">
        <SubHeading title="Awards & Recognition" />
        <h1 className="headtext__cormorant">Our Laurels</h1>

        <div className={styles.app__laurels_awards}>
          {data.awards.map((award) => (
            <AwardCard award={award} key={award.title} />
          ))}
        </div>
      </div>

      <div className="app__wrapper_img">
        <img src="/laurels.png" alt="laurels" />
      </div>
    </div>
  );
};

export default Laurels;
