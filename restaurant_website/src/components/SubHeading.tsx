interface Props {
  title: string;
}

const SubHeading = ({ title }: Props) => {
  return (
    <div style={{ marginBottom: '1rem' }}>
      <p className="p__cormorant">{title}</p>

      <img src="/spoon.png" alt="spoon" className="spoon_img" />
    </div>
  );
};

export default SubHeading;
